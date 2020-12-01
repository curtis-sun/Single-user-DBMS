# include "RecordManager.h"
# include <cassert>
# include <cstring>
# include "../utils/compare.h"

int RecordManager::createFile(){
    if (!fm->createFile(tableName.c_str())){
        return 1;
    }
    if (!fm->openFile(tableName.c_str(), fileID)){
        return 2;
    }
    slotCntPerPage = PAGE_SIZE / header->pminlen;

    allocPage();
    int index;
    BufType pagePos = bpm ->getPage(fileID, 1, index);
    memcpy(defaultRow, pagePos, header->pminlen);
    RID_t nextAvailable = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &nextAvailable, 8);
    return 0;
}

int RecordManager::destroyFile(){
    bpm ->close(fileID);
    if (!fm->destroyFile(tableName.c_str())){
        return 3;
    }
    fileID = -1;
    return 0;
}

int RecordManager::openFile(){
    if (fileID < 0){
        if (!fm->openFile(tableName.c_str(), fileID)){
            return 2;
        }
        int index;
        BufType headerPos = bpm ->getPage(fileID, 0, index);
        bpm->access(index);
        memcpy(header, headerPos, sizeof(*header));
        slotCntPerPage = PAGE_SIZE / header->pminlen;
        // 读取各列默认值
        memcpy(defaultRow, bpm ->getPage(fileID, 1, index), header->pminlen);
    }
    return 0;
}

int RecordManager::closeFile(){
    int index;
    BufType headerPos = bpm ->getPage(fileID, 0, index);
    memcpy(headerPos, header, sizeof(*header));
    bpm ->markDirty(index);
    BufType pagePos = bpm ->getPage(fileID, 1, index);
    memcpy(pagePos, defaultRow, header->pminlen);
    bpm ->markDirty(index);
    bpm ->close(fileID);
    if (fm ->closeFile(fileID)){
        return 4;
    }
    fileID = -1;
    return 0;
}

void RecordManager::allocPage() {
    int index;
    BufType pagePos = bpm ->allocPage(fileID, header->pageCnt, index);
    RID_t pageRID = header->pageCnt * slotCntPerPage;
    int p = 0;
    for (int i = 0; i < slotCntPerPage - 1; i ++, p += header->pminlen){
        *(RID_t *) (pagePos + p) = (RID_t) pageRID + i + 1;
    }
    *(RID_t *) (pagePos + p) = header->nextAvailable;
    header->nextAvailable = pageRID;
    bpm ->markDirty(index);
    header->pageCnt ++;
}

void RecordManager::insertRecord(const char* data){
    if (header->nextAvailable == (RID_t) -1){
        allocPage();
    }
    uint32_t pageID = header->nextAvailable / slotCntPerPage;
    uint16_t offset = header->nextAvailable - pageID * slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header->pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &nextAvailable, 8);
    memcpy(pagePos + 8, data, header->pminlen - 8);
    bpm -> markDirty(index);
}

void RecordManager::deleteRecord(RID_t rid){
    uint32_t pageID = rid / slotCntPerPage;
    uint16_t offset = rid - pageID * slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header->pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    *(RID_t *)pagePos = nextAvailable;
    bpm -> markDirty(index);
}

void RecordManager::updateRecord(RID_t rid, const char* data){
    uint32_t pageID = rid / slotCntPerPage;
    uint16_t offset = rid - pageID * slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header->pminlen;
    bpm ->access(index);
    memcpy(pagePos + 8, data, header->pminlen - 8);
    bpm -> markDirty(index);
}

void RecordManager::getRecord(RID_t rid, char* data){
    uint32_t pageID = rid / slotCntPerPage;
    uint16_t offset = rid - pageID * slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header->pminlen;
    bpm ->access(index);
    memcpy(data, pagePos + 8, header->pminlen - 8);
}

RecordManager::RecordManager(std::string path, TableHeader* m_header): tablePath(path), header(m_header){
    int err = mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if (err){
        throw "error: mkdir " + path + " fail";
    }
    tableName = path + "/tb";
    bpm = &BufPageManager::instance();
    fm = bpm->fileManager;

    fileScan = new RM_FileScan(this);
}

void RM_FileScan::openScan(AttrType m_attrType, int m_attrLength, int m_attrOffset, CompOp m_compOp, const char* m_value){
    attrType = m_attrType;
    attrLength = m_attrLength;
    attrOffset = m_attrOffset;
    compOp = m_compOp;
    memcpy(value, m_value + m_attrOffset, m_attrLength);
    pageID = 0;
    offset = rm->slotCntPerPage - 1;
}   

int RM_FileScan::getNextEntry(char* data){
    while(true){
        offset ++;
        if (offset >= rm->slotCntPerPage){
            pageID ++;
            offset = 0;
            if (pageID >= rm->header->pageCnt){
                pageID = rm->slotCntPerPage;
                return -1;
            }
            pagePos = rm->bpm ->getPage(rm->fileID, pageID, index);
        }
        else{
            pagePos += rm->header->pminlen;
        }
        RID_t rid = pageID * rm->slotCntPerPage + offset;
        if (rid != *(RID_t *)pagePos){
            continue;
        }
        memcpy(tempData, pagePos + attrOffset, attrLength);
        if (compare(tempData, value, attrType, compOp)){
            rm->bpm ->access(index);
            memcpy(data, tempData, attrLength);
            return 0;
        }
    }
} 