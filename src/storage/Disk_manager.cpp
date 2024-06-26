#include "storage/Disk_manager.h"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;
using namespace std;

Disk_manager::Disk_manager() : heapFile("FreeBlocksHF"), rootNode(disk.getRoot()) {
}

void Disk_manager::selectDiskStructure(bool defaultDisk) {
  if (defaultDisk) {
    dataDisk.push_back(4);
    dataDisk.push_back(8);
    dataDisk.push_back(2);
    disk.generateDiskStructure();
    disk.capacityDisk();
    /* Despues de generar la estructura del disco custom carga
     * el heapFile con los bloques libres
     */
    findFreeBlock();
    heapFile.saveToFileFreeBlocks();
  } else {
    int plates, tracks, sector, bytes, bytesPerBlock;
    cout << "Ingrese el número de platos: ";
    cin >> plates;
    cout << "Ingrese el número de pistas por superficie: ";
    cin >> tracks;
    cout << "Ingrese el número de sectores por pista: ";
    cin >> sector;
    cout << "Ingrese el número de bytes por sector: ";
    cin >> bytes;
    cout << "Ingrese el numero de bytes por bloque: ";
    cin >> bytesPerBlock;

    int sectorPerBlock = bytesPerBlock/bytes;
    int blockPerTrack = sector/sectorPerBlock;
    dataDisk.push_back(plates*2);
    dataDisk.push_back(tracks);
    dataDisk.push_back(blockPerTrack);

    disk = Disk(plates, tracks, sector, bytes, bytesPerBlock);
    disk.generateDiskStructure();
    disk.capacityDisk();

    /* Despues de generar la estructura del disco custom carga
     * el heapFile con los bloques libres
     */
    findFreeBlock();
    heapFile.saveToFileFreeBlocks();
  }
}

void Disk_manager::getCapacityDisk() {
  disk.capacityDisk();
}

string eliminarSubstring(string& str, const string& substr) {
  regex pattern(substr + "\\d*\\.txt");
  str = regex_replace(str, pattern, "");
  return str;
}

void Disk_manager::insertRecord(string &relation, string &record, int recordSize) {
  string blocksUsedPath = "../../data/heapfiles/" + relation + ".txt";
  ifstream bUsed(blocksUsedPath);

  string blockPath;
  if (bUsed.is_open()) {
    getline(bUsed, blockPath);
    bUsed.close();
  } else {
    blockPath = heapFile.getAndRemoveFirstBlock();
    HeapFile::addBlockToRelation(relation,blockPath);
    heapFile.saveToFileFreeBlocks();
  }

  if (blockPath.empty()) {
    cerr << "Error: No se encontró un bloque libre." << endl;
    return;
  }

  ofstream file(blockPath, std::ios::app);

  file << record << endl;

  file.close();

  cout << "Datos insertados en " + blockPath + " exitosamente Bv." << endl;

}

void Disk_manager::findFreeBlock() {
  searchFreeBlockInTree(rootNode);
}

void Disk_manager::searchFreeBlockInTree(const TreeNode &node) {
  if (isBlockFree(node.directory)) {
    heapFile.addFreeBlock(node.directory.string());
  }

  for (const auto &child : node.children) {
    searchFreeBlockInTree(child);
  }
}

bool Disk_manager::isBlockFree(const fs::path &blockPath) {
  ifstream blockFile(blockPath);

  if (blockFile.is_open()) {
    string firstLine;
    getline(blockFile, firstLine);

    blockFile.close();

    return (firstLine.find("FREE") != string::npos);
  }

  return false;
}

void Disk_manager::writeRecordInSectors(string &relation) {
  /* Aqui abro la relacion para saber de que bloque quiero escribir en el Disco (sectores)*/
  string blocksUsedPath = "../../data/heapfiles/" + relation + ".txt";
  ifstream bUsed(blocksUsedPath);

  string blockPath;
  if (bUsed.is_open()) {
    getline(bUsed, blockPath);
    bUsed.close();
  }

  /* Se optiene el nombre de la ruta para poder abrir los sectores correspondientes
   * y escribir en ellos
   */
  ifstream blockFile(blockPath);
  blockPath = eliminarSubstring(blockPath, "/block");

}


string Disk_manager::openBlock(const string &blockPath, const string &relationName) {
  fstream blockFile(blockPath, ios::in | ios::out);

  if (!blockFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << blockPath << "." << endl;
    return "";
  }

  string firstLine;
  getline(blockFile, firstLine);

  if (firstLine == "FREE") {
    blockFile.seekp(0);
    blockFile << relationName;
  }

  blockFile.close();

  return blockPath;
}

bool Disk_manager::checkSpaceInBlock(const string &blockPath, int recordSize) {
  ifstream blockFile(blockPath);

  if (!blockFile.is_open()) {
    cerr << "DM: No se pudo abrir el archivo " << blockPath << "." << endl;
    return false;
  }

  string line;
  while (getline(blockFile, line)) {
    if (line.substr(0, 13) == "blockCapacity") {
      int blockCapacity = stoi(line.substr(14));
      blockFile.close();
      return blockCapacity > recordSize;
    }
  }

  blockFile.close();
  return false;
}

string Disk_manager::redirectSectorWithSpace(const string &blockPath, int recordSize) {
  ifstream blockFile(blockPath);

  if (!blockFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << blockPath << "." << endl;
    return "";
  }

  string line;
  bool encontrado = false;

  while (getline(blockFile, line) && !encontrado) {
    if (line.substr(0, 1) == "C") {
      line = line.substr(1);
      int sectorCapacity = stoi(line.substr(line.find("#") + 1));
      if (sectorCapacity > recordSize) {
        blockFile.close();
        encontrado = true;
        return blockPath + "/" + line.substr(0, line.find("#"));
      }
    }
  }

  blockFile.close();
  return "";
}

vector<int> Disk_manager::getDataDisk() {
  return dataDisk;
}