# include "RecordManager.h"
# include <cassert>

void RecordManager::createFile(){
    fm->createFile(tablePath.c_str());
    fm->openFile(tablePath.c_str(), fileID);
    int index;
    bpm ->allocPage(fileID, 0, index);

    header->nextAvailable = (RID_t) -1;
    header->pageCnt = 1;
    header->pminlen = 8;
    header->slotCntPerPage = PAGE_SIZE / header->pminlen;
    header->columnCnt = 1;
    header->columnOffsets[0] = 0;
    header->columnNames[0][0] = 0;
}

void RecordManager::dropFile(){
    assert(fileID >= 0);
    fm ->closeFile(fileID);
}

void RecordManager::openFile(){
    fm->openFile(tablePath.c_str(), fileID);
    int index;
    BufType headerPos = bpm ->getPage(fileID, 0, index);
    bpm->access(index);
    strcpy(header, headerPos, sizeof(*header));
}

void RecordManager::closeFile(){
    int index;
    BufType headerPos = bpm ->getPage(fileID, 0, index);
    strcpy(headerPos, header, sizeof(*header));
    bpm ->markDirty(index);
    fm ->closeType(fileID);
}

void RecordManager::allocPage() {
    int index;
    BufType pagePos = bpm ->allocPage(fileID, header ->pageCnt, index);
    RID_t pageRID = header ->pageCnt * header ->slotCntPerPage;
    int p = 0;
    for (int i = 0; i < header ->slotCntPerPage - 1; i ++, p += header->pminlen){
        *(RID_t *) (pagePos + p) = (RID_t) pageRID + i + 1;
    }
    *(RID_t *) (pagePos + p) = header ->nextAvailable;
    header ->nextAvailable = pageRID;
    bpm ->markDirty(index);
    header ->pageCnt ++;
}

void RecordManager::insertRecord(const char* data){
    if (header ->nextAvailable == (RID_t) -1){
        allocPage();
    }
    int pageID = header ->nextAvailable / header ->slotCntPerPage;
    int offset = header ->nextAvailable - pageID * header ->slotCntPerPage;
    int index;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header ->pminlen;
    bpm ->access(index);
    header -> nextAvailable = *(RID_t *)pagePos;
    strcpy(pagePos, data, header ->pminlen);
    bpm -> markDirty(index);
}

void RecordManager::deleteRecord(RID_t rid){
    int pageID = rid / header ->slotCntPerPage;
    int offset = rid - pageID * header ->slotCntPerPage;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header ->pminlen;
    bpm ->access(index);
    RID_t nextAvailable = header -> nextAvailable;
    header -> nextAvailable = *(RID_t *)pagePos;
    *(RID_t *)pagePos = nextAvailable;
    bpm -> markDirty(index);
}

void RecordManager::updateRecord(RID_t rid, const char* data){
    int pageID = rid / header ->slotCntPerPage;
    int offset = rid - pageID * header ->slotCntPerPage;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header ->pminlen;
    bpm ->access(index);
    strcpy(pagePos, data, header ->pminlen);
    bpm -> markDirty(index);
}

void RecordManager::selectRecord(RID_t rid, char* data){
    int pageID = rid / header ->slotCntPerPage;
    int offset = rid - pageID * header ->slotCntPerPage;
    BufType pagePos = bpm ->getPage(fileID, pageID, index) + offset * header ->pminlen;
    bpm ->access(index);
    strcpy(data, pagePos, header ->pminlen);
}

RecordManager::RecordManager(std::string m_name, std::string path, BufPageManager* m_bpm){
    header = new TableHeader;
    tableName = m_name;

    tablePath = path + tableName + ".tb";

    assert(m_bpm != nullptr);
    bpm = m_bpm;
    fm = m_bpm->fileManager;
}