#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

typedef struct Page
{
	SM_PageHandle data; 
	PageNumber pageNum; 
	int dirtyBit; 
	int fixCount; 
	int hitNum;   
 	int refNum;   
} PageFrame;

SM_FileHandle fh;
int bufferSize = 0;
int rearIndex = 0;
int writeCount = 0;
int hit = 0;
int clockPointer = 0;
int lfuPointer = 0;

extern PageFrame helper(BM_BufferPool *const bm, PageFrame *page)
{
	PageFrame *pf = (PageFrame *) bm->mgmtData;
	return pf;
}

extern void build(const int numPages)
{
	int i;
	bufferSize = numPages;	
	PageFrame *page = malloc(sizeof(PageFrame) * numPages);
	for(i = bufferSize - 1; i >= 0; i--)
	{
		page[i].data = NULL;
		page[i].pageNum = -1;
		page[i].dirtyBit = 0;
		page[i].fixCount = 0;
		page[i].hitNum = 0;	
		page[i].refNum = 0;
	}
}

extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	int i, frontIndex;
	frontIndex = rearIndex % bufferSize;

	// Interating through all the page frames in the buffer pool
	for(i = 0; i < bufferSize; i++)
	{
		if(pageFrame[frontIndex].fixCount == 0)
		{
			if(pageFrame[frontIndex].dirtyBit == 1)
			{
				openPageFile(bm->pageFile, &fh);
				writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
				writeCount++;
			}
			pageFrame[frontIndex].data = page->data;
			pageFrame[frontIndex].pageNum = page->pageNum;
			pageFrame[frontIndex].dirtyBit = page->dirtyBit;
			pageFrame[frontIndex].fixCount = page->fixCount;
			break;
		}
		else
		{
			frontIndex++;
			frontIndex = (frontIndex % bufferSize == 0) ? 0 : frontIndex;
		}
	}
}

// Defining LFU (Least Frequently Used) function
extern void LFU(BM_BufferPool *const bm, PageFrame *page)
{
// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i, j, leastFreqIndex, leastFreqRef;
	leastFreqIndex = lfuPointer;	
	for(i = 0; i < bufferSize; i++)
	{
		if(pageFrame[leastFreqIndex].fixCount == 0)
		{
			leastFreqIndex = (leastFreqIndex + i) % bufferSize;
			leastFreqRef = pageFrame[leastFreqIndex].refNum;
			break;
		}
	}

	i = (leastFreqIndex + 1) % bufferSize;
	for(j = 0; j < bufferSize; j++)
	{
		if(pageFrame[i].refNum < leastFreqRef)
		{
			leastFreqIndex = i;
			leastFreqRef = pageFrame[i].refNum;
		}
		i = (i + 1) % bufferSize;
	}
	if(pageFrame[leastFreqIndex].dirtyBit == 1)
	{
		openPageFile(bm->pageFile, &fh);
		writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);
		
		// Increase the writeCount which records the number of writes done by the buffer manager.
		writeCount++;
	}
	pageFrame[leastFreqIndex].data = page->data;
	pageFrame[leastFreqIndex].pageNum = page->pageNum;
	pageFrame[leastFreqIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastFreqIndex].fixCount = page->fixCount;
	lfuPointer = leastFreqIndex + 1;
}

extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{	
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	int i, leastHitIndex, leastHitNum;
	for(i = 0; i < bufferSize; i++)
	{
		if(pageFrame[i].fixCount == 0)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
			break;
		}
	}	

	for(i = leastHitIndex + 1; i < bufferSize; i++)
	{
		if(pageFrame[i].hitNum < leastHitNum)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
		}
	}

	// If page in memory has been modified (dirtyBit = 1), then write page to disk
	if(pageFrame[leastHitIndex].dirtyBit == 1)
	{
		openPageFile(bm->pageFile, &fh);
		writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);
		writeCount++;
	}
	pageFrame[leastHitIndex].data = page->data;
	pageFrame[leastHitIndex].pageNum = page->pageNum;
	pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastHitIndex].fixCount = page->fixCount;
	pageFrame[leastHitIndex].hitNum = page->hitNum;
}

extern void CLOCK(BM_BufferPool *const bm, PageFrame *page)
{	
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	while(1)
	{
		clockPointer = (clockPointer % bufferSize == 0) ? 0 : clockPointer;

		if(pageFrame[clockPointer].hitNum == 0)
		{
			if(pageFrame[clockPointer].dirtyBit == 1)
			{
				openPageFile(bm->pageFile, &fh);
				writeBlock(pageFrame[clockPointer].pageNum, &fh, pageFrame[clockPointer].data);
				writeCount++;
			}
			pageFrame[clockPointer].data = page->data;
			pageFrame[clockPointer].pageNum = page->pageNum;
			pageFrame[clockPointer].dirtyBit = page->dirtyBit;
			pageFrame[clockPointer].fixCount = page->fixCount;
			pageFrame[clockPointer].hitNum = page->hitNum;
			clockPointer++;
			break;	
		}
		else
		{
			pageFrame[clockPointer++].hitNum = 0;		
		}
	}
}

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	FILE *f = fopen(pageFileName, "r+");;
	if (f == null) return RC_FILE_NOT_FOUND;
	else fclose(f);
	build(numPages); // *******************line 43 - 57	
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = page;
	writeCount = 0;
	clockPointer = 0;
	lfuPointer = 0;
	return RC_OK;
		
}

extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	if(bm->mgmtData == NULL)
	{
		return RC_NO_POOL;
	}
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pf = helper(bm, page);
	forceFlushPool(bm);
	int i;	
	for(i = bufferSize - 1; i >= 0; i--)
	{
		if (pageFrame[i].fixCount == 0) {}
		else
		{
			return RC_BUFFER_HAS_PAGES;
		}
	}N
	free(pf);
	bm->mgmtData = NULL;
	return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm)
{
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i = 0;	
	while (i < bufferSize)
	{
		if (pageFrame[i].fixCount != 0){}
		else if (pageFrame[i].dirtyBit != 1){}
		else
		{
			openPageFile(bm->pageFile, &fh);
			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
			pageFrame[i].dirtyBit = 0;
			writeCount++;
		}
		i++;
	}	
	return RC_OK;
}

extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	int i = 0;
	While(i < bufferSize)
	{
		if(pageFrame[i].pageNum == page->pageNum)
		{
			pageFrame[i].dirtyBit = 1;
			return RC_OK;		
		}	
		i++;
	}		
	return RC_ERROR;
}

extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{	
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i = 0;
	boolean done = false;
	while (i < bufferSize && !done)
	{
		if(pageFrame[i].pageNum == page->pageNum)
		{
			pageFrame[i].fixCount--;
			done = true;
		}
		i++;
	}
	return RC_OK;
}

extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i = 0;
	while(i < bufferSize)
	{
		if(pageFrame[i].pageNum != page->pageNum){}
		else
		{		
			openPageFile(bm->pageFile, &fh);
			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
			pageFrame[i].dirtyBit = 0;
			writeCount++;
		}
		i++;
	}	
	return RC_OK;
}

// extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
// 	    const PageNumber pageNum)
// {
// 	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
// 	PageFrame *pageFrame = helper(bm, page);
	
// 	// Checking if buffer pool is empty and this is the first page to be pinned
// 	if(pageFrame[0].pageNum == -1)
// 	{
// 		// Reading page from disk and initializing page frame's content in the buffer pool
// 		openPageFile(bm->pageFile, &fh);
// 		pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
// 		ensureCapacity(pageNum,&fh);
// 		readBlock(pageNum, &fh, pageFrame[0].data);
// 		pageFrame[0].pageNum = pageNum;
// 		pageFrame[0].fixCount++;
// 		rearIndex = hit = 0;
// 		pageFrame[0].hitNum = hit;	
// 		pageFrame[0].refNum = 0;
// 		page->pageNum = pageNum;
// 		page->data = pageFrame[0].data;
		
// 		return RC_OK;		
// 	}
// 	else
// 	{	
// 		int i;
// 		bool isBufferFull = true;
		
// 		for(i = 0; i < bufferSize; i++)
// 		{
// 			if(pageFrame[i].pageNum != -1)
// 			{	
// 				// Checking if page is in memory
// 				if(pageFrame[i].pageNum == pageNum)
// 				{
// 					// Increasing fixCount i.e. now there is one more client accessing this page
// 					pageFrame[i].fixCount++;
// 					isBufferFull = false;
// 					hit++; // Incrementing hit (hit is used by LRU algorithm to determine the least recently used page)

// 					if(bm->strategy == RS_LRU)
// 						// LRU algorithm uses the value of hit to determine the least recently used page	
// 						pageFrame[i].hitNum = hit;
// 					else if(bm->strategy == RS_CLOCK)
// 						// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
// 						pageFrame[i].hitNum = 1;
// 					else if(bm->strategy == RS_LFU)
// 						// Incrementing refNum to add one more to the count of number of times the page is used (referenced)
// 						pageFrame[i].refNum++;
					
// 					page->pageNum = pageNum;
// 					page->data = pageFrame[i].data;

// 					clockPointer++;
// 					break;
// 				}				
// 			} 
// 			else 
// 			{
// 				openPageFile(bm->pageFile, &fh);
// 				pageFrame[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
// 				readBlock(pageNum, &fh, pageFrame[i].data);
// 				pageFrame[i].pageNum = pageNum;
// 				pageFrame[i].fixCount = 1;
// 				pageFrame[i].refNum = 0;
// 				rearIndex++;	
// 				hit++; // Incrementing hit (hit is used by LRU algorithm to determine the least recently used page)

// 				if(bm->strategy == RS_LRU)
// 					// LRU algorithm uses the value of hit to determine the least recently used page
// 					pageFrame[i].hitNum = hit;				
// 				else if(bm->strategy == RS_CLOCK)
// 					// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
// 					pageFrame[i].hitNum = 1;
						
// 				page->pageNum = pageNum;
// 				page->data = pageFrame[i].data;
				
// 				isBufferFull = false;
// 				break;
// 			}
// 		}
		
// 		// If isBufferFull = true, then it means that the buffer is full and we must replace an existing page using page replacement strategy
// 		if(isBufferFull == true)
// 		{
// 			// Create a new page to store data read from the file.
// 			PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));		
			
// 			// Reading page from disk and initializing page frame's content in the buffer pool
// 			openPageFile(bm->pageFile, &fh);
// 			newPage->data = (SM_PageHandle) malloc(PAGE_SIZE);
// 			readBlock(pageNum, &fh, newPage->data);
// 			newPage->pageNum = pageNum;
// 			newPage->dirtyBit = 0;		
// 			newPage->fixCount = 1;
// 			newPage->refNum = 0;
// 			rearIndex++;
// 			hit++;

// 			if(bm->strategy == RS_LRU)
// 				// LRU algorithm uses the value of hit to determine the least recently used page
// 				newPage->hitNum = hit;				
// 			else if(bm->strategy == RS_CLOCK)
// 				// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
// 				newPage->hitNum = 1;

// 			page->pageNum = pageNum;
// 			page->data = newPage->data;			

// 			// Call appropriate algorithm's function depending on the page replacement strategy selected (passed through parameters)
// 			switch(bm->strategy)
// 			{			
// 				case RS_FIFO: // Using FIFO algorithm
// 					FIFO(bm, newPage);
// 					break;
				
// 				case RS_LRU: // Using LRU algorithm
// 					LRU(bm, newPage);
// 					break;
				
// 				case RS_CLOCK: // Using CLOCK algorithm
// 					CLOCK(bm, newPage);
// 					break;
  				
// 				case RS_LFU: // Using LFU algorithm
// 					LFU(bm, newPage);
// 					break;
  				
// 				case RS_LRU_K:
// 					printf("\n LRU-k algorithm not implemented");
// 					break;
				
// 				default:
// 					printf("\nAlgorithm Not Implemented\n");
// 					break;
// 			}
						
// 		}		
// 		return RC_OK;
// 	}	
// }

extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferSize);
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	int i;
	for(i = 0; i < bufferSize; i++) 
	{
		if (pageFrame[i].pageNum == -1) frameContents[i] = NO_PAGE;
		else frameContents[i] = pageFrame[i].pageNu;
	}
	return frameContents;
}

extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
	bool *dirtyFlags = malloc(sizeof(bool) * bufferSize);
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i = 0;
	// Iterating through all the pages in the buffer pool and setting dirtyFlags' value to TRUE if page is dirty else FALSE
	while(i < bufferSize)
	{
		if (pageFrame[i].dirtyBit != 1) dirtyFlags[i] = false ;
		else dirtyFlags[i] = true;
		i++;
	}
	return dirtyFlags;
}

extern int *getFixCounts (BM_BufferPool *const bm)
{
	int *fixCounts = malloc(sizeof(int) * bufferSize);
	// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData    ******************** goes to helper line 38 - 41
	PageFrame *pageFrame = helper(bm, page);
	
	int i = 0;
	// Iterating through all the pages in the buffer pool and setting fixCounts' value to page's fixCount
	while(i < bufferSize)
	{
		if (pageFrame[i].fixCount == -1) fixCounts[i] = 0;
		else fixCounts[i] = pageFrame[i].fixCount;
		i++;
	}	
	return fixCounts;
}

extern int getNumReadIO (BM_BufferPool *const bm)
{
	return (rearIndex + 1);
}

extern int getNumWriteIO (BM_BufferPool *const bm)
{
	return writeCount;
}
