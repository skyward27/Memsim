#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"
//Basic struct for holding all the info needed for each page frame
struct memPage{
    unsigned addr;
    int dirty;
    int clean;
    struct memPage* prev;
    struct memPage* next;
};
//Struct needed for the clean and dirty lists in vms.
struct list{
    unsigned addr;
    struct list* next;
    struct list* prev;
};
struct list* addToListCD(unsigned newAddr, struct list* head){
    if(head->addr == 0){
        if(head->next != 0) free(head->next);
        head->addr = newAddr;
        head->next = 0;
        head->prev = 0;
        return head;
    }
    else{
        struct list *curr = head;
        while(curr->next !=0){
            curr = curr->next;
        }
        struct list *new = malloc(sizeof(*new));
        new->addr = newAddr;
        new->next = 0;
        new->prev = curr;
        curr->next = new;
        return head;
    }
}
//Debug function to print all the page frames in the current memory list
void printList(struct memPage* head){
    struct memPage* curr = head;
    while(curr !=0){
        printf("%x ", curr->addr);
        curr = curr->next;
    }
    printf("\n");
}
//Takes in a new memory address and an existing list and adds the frame to the list, with an addition tag for if its dirty bit should be activated
struct memPage* addToList(unsigned newAddr, struct memPage* head, int write){
    if(head->next == 0){
        struct memPage *new = malloc(sizeof(*new));
        head->next = new;
        new->addr = newAddr;
        new->clean = 0;
        new->dirty = 0;
        new->next = 0;
        new->prev = head;
        if(write == 1){
            new->dirty = 1;
        }
        return new;
    }
    else{
        struct memPage* curr = head;
        while(curr->next != 0){
            curr = curr->next;
        }
        struct memPage *new = malloc(sizeof(*new));
        new->addr = newAddr;
        new->clean = 0;
        new->dirty = 0;
        new->next = 0;
        new->prev = curr;
        curr->next = new;
        if(write == 1){
            new->dirty = 1;
        }
        return new;
    }
}
//Trys to find an address in the list given to it. If not found, returns 0. IF found, returns 1 and if write is 1, updates the dirty bit for that entry
int findAddr(struct memPage *head, unsigned addr, int write){
    struct memPage* curr = head;
    if(head->next == NULL && head->addr != addr){
        return 0;
    }
    while(curr != 0){
        if(curr->addr == addr){
            if(write == 1){
                curr->dirty = 1;
            }
            return 1;
        }
        curr= curr->next;
    }
    return 0;
}

int findAddrCD(struct list *head, unsigned addr){
    struct list *curr = head;
    if(head->next == 0 &&head->addr != addr){
        return 0;
    }
    while(curr !=0){
        if(curr->addr == addr) return 1;
        curr = curr->next;
    }
    return 0;
}
struct list* deleteFifo(struct list *head, int *addrToReplace){
    if(head->next == 0){
        *addrToReplace = head->addr;
        head->addr = 0;
        return head;
    }
    else{
        *addrToReplace = head->addr;
        struct list *curr = head;
        head = head->next;
        free(curr);
        return head;
    }
}
struct list* deleteAddr(struct list* head, int addr){
    if(head->next == 0){
        head->addr = 0;
        return head;
    }
    else if( head->next !=0 && head->addr == addr){
        struct list* curr = head;
        head = head->next;
        free(curr);
        return head;
    }
    else{
        struct list* curr, *prev, *next;
        curr = head;
        next = head->next;
        prev = 0;
        while(curr!= 0){
            next = curr->next;
            prev = curr->prev;
            if(curr->addr == addr){
                
                if(next == NULL){
                    prev->next = 0;
                }
                else{
                    prev->next = next;
                    next->prev = prev;
                }
               free(curr);
               return head;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    return head;
}
//Function to return the tail of the linked list. Needed for removals and addtions.
struct memPage* getTail(struct memPage *head){
    struct memPage* curr = head;
    while(curr->next != 0){
        curr= curr->next;
    }
    return curr;
}
/*Moves the given address to the back of the linked list given. Does nothing if the list is empty or it cannot find the adddress.
Needed to update the list for the lru algorithm as the back of the list is treated like the front of the queue.*/
struct memPage* updateTail(unsigned addr, struct memPage *head, struct memPage *tail, int write){
    struct memPage *curr, *next, *prev, *newhead, *newtail;
    if(head->next == 0){
        printf("\nEmpty List.\n");
        return head;
    }
    if(head->addr == addr){
        tail->next = head;
        head->prev = tail;
        head = head->next;
        tail = tail->next;
        tail->next = 0;
        if(write == 1){
            tail->dirty = 1;
        }
        head->prev = 0;
        return head;
    }
    if(tail->addr == addr) return head;
    curr = head;
    next = head->next;
    prev = 0;
    while(curr != 0){
        if(curr->addr == addr){
            next->prev = prev;
            prev->next = next;
            curr->next = 0;
            curr->prev = tail;
            tail->next = curr;
            if(write == 1){
                curr->dirty = 1;
            }
            curr = getTail(head);
            return head;
        }
        else{
            prev = curr;
            curr = curr-> next;
            next = curr->next;
        }
    }
    printf("Error: Memory address not found!\n");
    curr = getTail(head);
    return curr;
}
//Replaces a random page in the given linked list with the given address. Checks if a disk write needs to occur and updates the total reads and writes.
void replaceRandom(struct memPage* head, unsigned addr, int randNum, int* diskWrites, int write){
    int i = 1;
    struct memPage* curr;
    curr = head;
    for(i = 1;i<randNum-1;i++){
        curr = curr->next;
    }
    curr->addr = addr;
    if(curr->dirty == 1){
        *diskWrites = *diskWrites + 1;
    }
    if(write == 1){
        curr->dirty = 1;
    }
    else{
        curr->dirty = 0;
    }
    curr->clean = 0;
    return;
}
//Replaces the last page in the list with the given address. Updates the total disk reads and writes as necessary. The last page is
//considering to be the start of the queue.
struct memPage* replaceFIFO(struct memPage* head, struct memPage* tail, unsigned newAddr, int* diskWrites, int write){
    struct memPage* curr;
    curr = head->next;
    if(head->dirty ==1){
        *diskWrites = *diskWrites + 1;
    }
    curr->prev = 0;
    head->addr = newAddr;
    head->prev = tail;
    tail->next = head;
    head->clean = 0;
    head->next = 0;
    if(write == 1){
        head->dirty = 1;
    }
    else{
        head->dirty = 0;
    }
    tail = head;
    head= curr;
    return head;
}
struct memPage* replacePage(struct memPage* head, unsigned addr){
    if(head->addr == addr){
        head->addr = addr;
        return head;
    }
    else{
        struct memPage* curr = head;
            while(curr!= 0){
                if(curr->addr == addr){
                    curr-> addr = addr;
                    return head;
                }
                else{
                    curr = curr->next;
                }
            }
    }
    return head;
}
struct memPage* replaceFIFOvms(struct memPage* head, struct memPage* tail, unsigned newAddr, int* diskWrites, int write, int *globalWrite, int *replacedAddr){
    struct memPage* curr;
    curr = head->next;
    if(head->dirty ==1){
        *globalWrite = 1;
    }
    else *globalWrite = 0;
    curr->prev = 0;
    *replacedAddr = head->addr;
    head->addr = newAddr;
    head->prev = tail;
    tail->next = head;
    head->clean = 0;
    head->next = 0;
    if(write == 1){
        head->dirty = 1;
    }
    else{
        head->dirty = 0;
    }
    tail = head;
    head= curr;
    return head;
}
/*Algorithm for random memory page replacement. First fills the list to the page frame capacity then replaces pages randomly using the random 
replacement function.*/
void rdm(FILE *memFile, int numFrames, int debug){
    int counter, diskRead, diskWrite, frameCount, write, randNum,flag, listSize;
    frameCount = diskRead = diskWrite = write = 0;
    unsigned addr;
    char rw;
    fscanf(memFile, "%x %c", &addr, &rw);
    struct memPage *head = malloc(sizeof(*head));
    head->addr = addr;
    head->clean = 0;
    head->prev = 0;
    head->next = 0;
    if(rw == 'W'||rw == 'w'){
        head->dirty = 1;
    }
    else{
        head->dirty = 0;
    }
    struct memPage* tail = head;
    diskRead++;
    frameCount++;
    listSize = 1;
    if(debug == 1){
        printf("Disk access for address %x, asking for a %c.\n", addr, rw);
    }
    srand(time(0));
    while(1){
        flag = fscanf(memFile,"%x %c", &addr, &rw);
        if(flag == EOF) break;
        if(rw == 'w'||rw=='W'){
            write = 1;
        }
        else{
            write = 0;
        }
        if(findAddr(head, addr, write) == 1){
            if(debug == 1){
                printf("Address %x is currently loaded in the page frame table.\n", addr);
            }
            frameCount++;
            continue;
        }
        else{
            if(listSize < numFrames){
                tail = addToList(addr, head, write);
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
                diskRead ++;
                listSize++;
            }
            else{
                
                randNum = (rand()%(numFrames-1+1))+1;
                if(debug == 1){
                    printf("Random number is: %d\n",randNum);
                }
                replaceRandom(head, addr, randNum,&diskWrite,write);
                diskRead++;
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
            }
            frameCount++;
        }
    }
    
    printf("Trace file completely read.\nTotal memory frames: %d\nEvents in trace: %d\nTotal disk reads: %d\nTotal disk writes: %d\n",numFrames,frameCount,diskRead,diskWrite);
}
/*Algorithm for replacing the memory pages in fifo order. Adds pages to the linked list until it reaches page frame capacity. Then replaces frames
by pulling from the end of the queue (head of the list) and moving the new frams to the back of the queue (tail of the list).*/
void fifo(FILE *memFile, int numFrames, int debug){
    int counter, diskRead, diskWrite, frameCount, write, randNum,flag, listSize;
    frameCount = diskRead = diskWrite = write = 0;
    unsigned addr;
    char rw;
    fscanf(memFile, "%x %c", &addr, &rw);
    struct memPage *head = malloc(sizeof(*head));
    head->addr = addr;
    head->clean = 0;
    head->prev = 0;
    head->next = 0;
    if(rw == 'W'||rw == 'w'){
        head->dirty = 1;
    }
    else{
        head->dirty = 0;
    }
    struct memPage* tail = head;
    listSize = 1;
    diskRead++;
    frameCount++;
    while(1){
        flag = fscanf(memFile,"%x %c", &addr, &rw);
        if(flag == EOF) break;
        if(rw == 'w'||rw=='W'){
            write = 1;
        }
        else{
            write = 0;
        }
        if(findAddr(head, addr, write) == 1){
            if(debug == 1){
                printf("Address %x is currently loaded in the page frame table.\n", addr);
            }
            frameCount++;
            continue;
        }
        else{
            if(listSize < numFrames){
                tail = addToList(addr, head, write);
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
                diskRead ++;
                listSize++;
            }
            else{
                head = replaceFIFO(head, tail, addr, &diskWrite, write);
                tail = getTail(head);
                diskRead++;
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
            }
            frameCount++;
        }
    }
    printf("Trace file completely read.\nTotal memory frames: %d\nEvents in trace: %d\nTotal disk reads: %d\nTotal disk writes: %d\n",numFrames,frameCount,diskRead,diskWrite);
}
/*Algorithm for least recently used. Essentially the same as fifo, but if a memory page is found already exisiting in the current memory list, 
it is moved to the back of the queue (the tail of the list) to simulate it being accessed so it is not replaced.*/
void lru(FILE *memFile, int numFrames, int debug){
    int counter, diskRead, diskWrite, frameCount, write, randNum,flag,listSize;
    frameCount = diskRead = diskWrite = write = 0;
    unsigned addr;
    char rw;
    fscanf(memFile, "%x %c", &addr, &rw);
    struct memPage *head = malloc(sizeof(*head));
    head->addr = addr;
    head->clean = 0;
    head->prev = 0;
    head->next = 0;
    if(rw == 'W'||rw == 'w'){
        head->dirty = 1;
    }
    else{
        head->dirty = 0;
    }
    struct memPage* tail = head;
    diskRead++;
    frameCount++;
    listSize = 1;
    while(1){
        flag = fscanf(memFile,"%x %c", &addr, &rw);
        if(flag == EOF) break;
        if(rw == 'w'||rw=='W'){
            write = 1;
        }
        else{
            write = 0;
        }
        if(findAddr(head, addr, write) == 1){
            if(debug == 1){
                printf("Address %x is currently loaded in the page frame table.\n", addr);
            }
            head = updateTail(addr,head, tail, write);
            tail = getTail(head);
            frameCount++;
            continue;
        }
        else{
            if(listSize < numFrames){
                tail = addToList(addr, head, write);
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
                diskRead ++;
                listSize++;
            }
            else{
                head = replaceFIFO(head, tail, addr, &diskWrite, write);
                tail = getTail(head);
                diskRead++;
                if(debug == 1){
                    printf("Disk access for address %x, asking for a %c.\n", addr, rw); 
                }
            }
            frameCount++;
            
        }
    }
    printf("Trace file completely read.\nTotal memory frames: %d\nEvents in trace: %d\nTotal disk reads: %d\nTotal disk writes: %d\n",numFrames,frameCount,diskRead,diskWrite);
}

void vms(FILE* memFile, int numFrames, int debug){
    int globalWrite;
    struct list *cleanHead, *dirtyHead;
    cleanHead = malloc(sizeof(*cleanHead));
    cleanHead->addr = 0;
    cleanHead->next = 0;
    cleanHead->prev = 0;
    dirtyHead = malloc(sizeof(*dirtyHead));
    dirtyHead->addr = 0;
    dirtyHead->next = 0;
    dirtyHead->prev = 0;
    globalWrite = 0;
    int rss1, rss2, even, p1Size, p2Size, write, frameCount,diskRead,diskWrite,sizeT,flag;
    diskRead = diskWrite = 0;
    p1Size = p2Size = 0;
    struct memPage *head1, *head2, *headT, *tail1, *tail2, *tailT;
    unsigned newAddr, replacedAddr,addrtoReplace;
    replacedAddr = 0;
    char rw;
    sizeT = 0;
    headT = 0;
    if(numFrames%2 == 0){
        rss1=rss2=(numFrames/2);
    }
    else{
        rss1 = (numFrames/2)+1;
        rss2 = numFrames/2;
    }
    while(1){
        flag = fscanf(memFile, "%x %c",&newAddr, &rw);
        if(flag == EOF){
            break;
        }
        if(newAddr >= 805306368 && newAddr <1073741824){
            if(p2Size == 0){
                head2 = malloc(sizeof(*head2));
                head2->addr = newAddr;
                head2->clean = 0;
                head2->next = 0;
                head2->prev = 0;
                if(rw == 'W'){
                    head2->dirty = 1;
                }
                else head2->dirty = 0;
                tail2 = head2;
                p2Size++;
                frameCount++;
            }
            else{
            if(rw == 'W'){
                write = 1;
            }
            else{
                write = 0;
            }
            if(findAddr(head2,newAddr,write) == 1){
                frameCount++;
            }
            else{
                if(p2Size < rss2){
                    tail2 = addToList(newAddr,head2, write);
                    p2Size++;
                }
                else{
                    head2 = replaceFIFOvms(head2, tail2, newAddr, &diskWrite, write,&globalWrite,&replacedAddr);
                    tail2 = getTail(head2);
                }
                frameCount++;
            }}
        }
        else{
            if(p1Size == 0){
                head1 = malloc(sizeof(*head1));
                head1->addr = newAddr;
                head1->clean = 0;
                head1->next = 0;
                head1->prev = 0;
                if(rw == 'W'){
                    head1->dirty = 1;
                }
                else head1->dirty = 0;
                tail1 = head1;
                p1Size++;
                frameCount++;
            }
            else{
            if(rw == 'W'){
                write = 1;
            }
            else{
                write = 0;
            }
            if(findAddr(head1,newAddr,write) == 1){
                frameCount++;
            }
            else{
                if(p1Size < rss1){
                    tail1 = addToList(newAddr,head1, write);
                    p1Size++;
                }
                else{
                    head1 = replaceFIFOvms(head1, tail1, newAddr, &diskWrite, write, &globalWrite, &replacedAddr);
                    tail1 = getTail(head1);
                }
                frameCount++;
            }}
        }
        if(sizeT == 0){
            headT = malloc(sizeof(*headT));
            headT->addr = newAddr;
            headT->clean = 0;
            headT->next = 0;
            headT->prev = 0;
            if(rw == 'W') headT->dirty = 1;
            else headT->dirty = 0;
            sizeT++;
            diskRead++;
            continue;
        }
        if(replacedAddr != 0){
            if(globalWrite == 1){
                dirtyHead = addToListCD(replacedAddr, dirtyHead);
            }
            else{
                cleanHead = addToListCD(replacedAddr,cleanHead);
            }
        }
        if(findAddr(headT,newAddr,write) == 1){
            if(findAddrCD(cleanHead,newAddr) == 1){
                deleteAddr(cleanHead, newAddr);
            }
            if(findAddrCD(dirtyHead,newAddr) == 1){
                deleteAddr(dirtyHead,newAddr);
            }
            continue;
        }
        else{
            if(sizeT<numFrames){
                if(rw == 'W') write = 1;
                else write = 0;
                tailT = addToList(newAddr,headT,write);
                sizeT++;
                diskRead++;
            }
            else{
                if(cleanHead->addr !=0){
                    addrtoReplace = cleanHead->addr;
                    cleanHead = deleteAddr(cleanHead,addrtoReplace);
                    headT = replacePage(headT,addrtoReplace);
                    diskRead++;
                }
                else{
                    addrtoReplace = dirtyHead->addr;
                    dirtyHead = deleteAddr(dirtyHead,addrtoReplace);
                    headT = replacePage(headT,addrtoReplace);
                    diskRead++;
                    diskWrite++;
                }
            }
        }
        replacedAddr = 0;
        globalWrite = 0;
    }
    printf("Trace file completely read.\nTotal memory frames: %d\nEvents in trace: %d\nTotal disk reads: %d\nTotal disk writes: %d\n",numFrames,frameCount,diskRead,diskWrite);
}

int main(int argc, char **argv){
    if(argc != 5){
        printf("\nInsufficient amount of command line arguments. The correct format is \".\\memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>");
        return 1;
    }
    FILE *trace;
    trace = fopen(argv[1],"r");
    int nFrames;
    nFrames = atoi(argv[2]);
    int debug;
    if(strcmp("debug",argv[4]) == 0 || strcmp("Debug",argv[4]) == 0 || strcmp("DEBUG",argv[4]) == 0){
        debug = 1;
    }
    else{
        debug = 0;
    }
    if(strcmp("rdm",argv[3]) == 0){
        rdm(trace, nFrames, debug);
        return 0;
    }
    if(strcmp("fifo",argv[3]) == 0){
        fifo(trace, nFrames, debug);
        return 0;
    }
    if(strcmp("lru",argv[3]) == 0){
        lru(trace, nFrames, debug);
        return 0;
    }
    if(strcmp("vms",argv[3]) == 0){
        vms(trace,nFrames,debug);
        return 0;
    }
}