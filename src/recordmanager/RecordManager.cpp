# include "RecordManager.h"
# include <cassert>
# include <cstring>
# include "../utils/compare.h"

void RecordManager::createFile(){
    fm->createFile(tablePath.c_str());
    fm->openFile(tablePath.c_str(), fileID);

    header.nextAvailable = (RID_t) -1;
    header.pageCnt = 1;
    header.pminlen = 8;
    header.slotCntPerPage = PAGE_SIZE / header.pminlen;
    header.columnCnt = 1;
    header.columnOffsets[0] = 0;
    header.columnOffsets[1] = header.pminlen;
    header.columnNames[0][0] = 0;
}

void RecordManager::destroyFile(){
    bpm ->close(fileID);
    fm ->destroyFile(tablePath.c_str());
    fileID = -1;
}

void RecordManager::openFile(){
    if (fileID < 0){
        fm->openFile(tablePath.c_str(), fileID);
        int index;
        BufType headerPos = bpm ->getPage(fileID, 0, index);
        bpm->access(index);
        memcpy(&header, headerPos, sizeof(header));
    }
}

void RecordManager::closeFile(){
    int index;
    BufType headerPos = bpm ->getPage(fileID, 0, index);
    memcpy(headerPos, &header, sizeof(header));
    bpm ->markDirty(index);
    bpm ->close(fileID);
    fm ->closeFile(fileID);
    fileID = -1;
}

void RecordManager::allocPage() {
    int index;
    BufType pagePos = bpm ->allocPage(fileID, header.pageCnt, index);
    RID_t pageRID = header.pageCnt * header.slotCntPerPage;
    int p = 0;
    for (int i = 0; i < header.slotCntPerPage - 1; i ++, p += header.pminlen){
        *(RID_t *) (pagePos + p) = (RID_t) pageRID + i + 1;
    }
    *(RID_t *) (pagePos + p) = header.nextAvailable;
    header.nextAvailable = pageRID;
    bpm ->markDirty(index);
    header.pageCnt ++;
}

void RecordManager::insertRecord(const char* data){
    if (header.nextAvailable == (RID_t) -1){
        allocPage();
    }
    uint32_t pageID = header.nextAvailable / header.slotCntPerPage;
    uint16_t offset = header.nextAvailable - pageID * header.slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header.pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header.nextAvailable;
    header.nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &nextAvailable, 8);
    memcpy(pagePos + 8, data, header.pminlen - 8);
    bpm -> markDirty(index);
}

void RecordManager::deleteRecord(RID_t rid){
    uint32_t pageID = rid / header.slotCntPerPage;
    uint16_t offset = rid - pageID * header.slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header.pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header.nextAvailable;
    header.nextAvailable = *(RID_t *)pagePos;
    *(RID_t *)pagePos = nextAvailable;
    bpm -> markDirty(index);
}

void RecordManager::updateRecord(RID_t rid, const char* data){
    uint32_t pageID = rid / header.slotCntPerPage;
    uint16_t offset = rid - pageID * header.slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header.pminlen;
    bpm ->access(index);
    memcpy(pagePos + 8, data, header.pminlen - 8);
    bpm -> markDirty(index);
}

void RecordManager::getRecord(RID_t rid, char* data){
    uint32_t pageID = rid / header.slotCntPerPage;
    uint16_t offset = rid - pageID * header.slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header.pminlen;
    bpm ->access(index);
    memcpy(data, pagePos + 8, header.pminlen - 8);
}

RecordManager::RecordManager(std::string m_name, std::string path, BufPageManager* m_bpm){
    tablePath = path + m_name + ".tb";

    assert(m_bpm != nullptr);
    bpm = m_bpm;
    fm = m_bpm->fileManager;

    fileScan = new RM_FileScan(this);
}

void RM_FileScan::openScan(AttrType m_attrType, int m_attrLength, int m_attrOffset, CompOp m_compOp, const char* m_value){
    attrType = m_attrType;
    attrLength = m_attrLength;
    attrOffset = m_attrOffset;
    compOp = m_compOp;
    memcpy(value, m_value + m_attrOffset, m_attrLength);
    pageID = 0;
    offset = rm->header.slotCntPerPage - 1;
}   

int RM_FileScan::getNextEntry(char* data){
    while(true){
        offset ++;
        if (offset >= rm->header.slotCntPerPage){
            pageID ++;
            offset = 0;
            if (pageID >= rm->header.pageCnt){
                pageID = rm->header.slotCntPerPage;
                return -1;
            }
            pagePos = rm->bpm ->getPage(rm->fileID, pageID, index);
        }
        else{
            pagePos += rm->header.pminlen;
        }
        RID_t rid = pageID * rm->header.slotCntPerPage + offset;
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