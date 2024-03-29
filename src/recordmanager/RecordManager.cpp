# include "RecordManager.h"
# include <cassert>
# include <cstring>

int RecordManager::createFile(){
    int err = mkdir(tablePath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    /*
    if (err){
        throw "error: mkdir " + tablePath + " fail";
    }*/
    if (!fm->createFile(tableName.c_str())){
        return 1;
    }
    if (!fm->openFile(tableName.c_str(), fileID)){
        return 2;
    }
    slotCntPerPage = PAGE_SIZE / header->pminlen;

    allocPage();
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, 1, index);
    //memcpy(defaultRow, pagePos, header->pminlen);
    RID_t nextAvailable = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &nextAvailable, 8);
    return 0;
}

int RecordManager::destroyFile(){
    closeFile();
    if (!fm->destroyFile(tableName.c_str())){
        return 3;
    }
    return 0;
}

int RecordManager::openFile(){
    if (fileID < 0){
        if (!fm->openFile(tableName.c_str(), fileID)){
            return 2;
        }
        int index;
        char* headerPos = (char*)bpm ->getPage(fileID, 0, index);
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
    char* headerPos = (char*)bpm ->getPage(fileID, 0, index);
    memcpy(headerPos, header, sizeof(*header));
    bpm ->markDirty(index);
    char* pagePos = (char*)bpm ->getPage(fileID, 1, index);
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
    char* pagePos = (char*)bpm ->allocPage(fileID, header->pageCnt + 1, index);
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

void RecordManager::insertRecord(const char* data, RID_t& rid){
    if (header->nextAvailable == (RID_t) -1){
        allocPage();
    }
    uint32_t pageId = header->nextAvailable / slotCntPerPage;
    uint16_t offset = header->nextAvailable - pageId * slotCntPerPage;
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, pageId + 1, index) + offset * header->pminlen;
    bpm ->access(index);
    rid = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &rid, 8);
    memcpy(pagePos + 8, data + 8, header->pminlen - 8);
    bpm -> markDirty(index);
}

/*
void RecordManager::insertRecord(const char* data, RID_t rid){
    uint32_t pageId = header->nextAvailable / slotCntPerPage;
    uint16_t offset = header->nextAvailable - pageId * slotCntPerPage;
    while (pageId >= header->pageCnt){
        allocPage();
    }
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, pageId + 1, index) + offset * header->pminlen;
    bpm ->access(index);
    header->nextAvailable = *(RID_t *)pagePos;
    memcpy(pagePos, &rid, 8);
    memcpy(pagePos + 8, data, header->pminlen - 8);
    bpm -> markDirty(index);
}*/

void RecordManager::deleteRecord(RID_t rid){
    uint32_t pageId = rid / slotCntPerPage;
    uint16_t offset = rid - pageId * slotCntPerPage;
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, pageId + 1, index) + offset * header->pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header->nextAvailable;
    header->nextAvailable = *(RID_t *)pagePos;
    *(RID_t *)pagePos = nextAvailable;
    bpm -> markDirty(index);
}

void RecordManager::updateRecord(RID_t rid, const char* data){
    uint32_t pageId = rid / slotCntPerPage;
    uint16_t offset = rid - pageId * slotCntPerPage;
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, pageId + 1, index) + offset * header->pminlen;
    bpm ->access(index);
    memcpy(pagePos, data, header->pminlen);
    bpm -> markDirty(index);
}

void RecordManager::getRecord(RID_t rid, char* data){
    uint32_t pageId = rid / slotCntPerPage;
    uint16_t offset = rid - pageId * slotCntPerPage;
    int index;
    char* pagePos = (char*)bpm ->getPage(fileID, pageId + 1, index) + offset * header->pminlen;
    bpm ->access(index);
    memcpy(data, pagePos, header->pminlen);
}

RecordManager::RecordManager(const std::string& path, TableHeader* m_header, const std::string& name): tablePath(path), tableName(name), header(m_header){
    bpm = BufPageManager::instance();
    fm = bpm->fileManager;

    fileScan = new RM_FileScan(this);
}

void RM_FileScan::openScan(){
    pageId = 0;
    offset = 0;
    pagePos = (char*)rm->bpm ->getPage(rm->fileID, 1, index);
}   

int RM_FileScan::getNextEntry(RID_t& rid){
    while(true){
        offset ++;
        if (offset >= rm->slotCntPerPage){
            pageId ++;
            offset = 0;
            if (pageId >= rm->header->pageCnt){
                pageId = rm->slotCntPerPage;
                return -1;
            }
            pagePos = (char*)rm->bpm ->getPage(rm->fileID, pageId + 1, index);
        }
        else{
            pagePos += rm->header->pminlen;
        }
        rid = pageId * rm->slotCntPerPage + offset;
        if (rid != *(RID_t *)pagePos){
            continue;
        }
        rm->bpm ->access(index);
        return 0;
    }
} 