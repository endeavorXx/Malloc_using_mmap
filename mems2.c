#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

#define PAGE_SIZE 4096

typedef unsigned long ulong;

// Define the structure for the main chain node
struct mainNode {
    struct subNode* subn;
    struct mainNode* prev;
    struct mainNode* next;
    void* start_virt_add;
    void* end_virt_add;
    void* start_add; // physical address
    int pages;
};

// Define the structure for the sub-chain node
struct subNode {
    struct mainNode* main_chain_node;
    struct subNode* prev;
    struct subNode* next;
    int status; // status-1(for process and 0 for hole)
    void* start_virt;                                        
    void* end_virt;
    void* phy_add;
};

// Global variables
struct mainNode* head = NULL;
struct mainNode* tail = NULL;
void* last_virt_add = NULL;
void* p = NULL;
int No_of_pages = 0;
int count = 0;                      //count no of nodes in the main chain
/*
Initializes all the required parameters for the MeMS system.
*/
void mems_init() {
    head = (struct mainNode*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (head == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    head->prev = NULL;
    head->next = NULL;
    head->end_virt_add = NULL;
    head->subn = NULL;
    head->pages = 0;
    head->start_add = (void *)head;
    // tail = head;
    printf("Head Physical address is:%d\n", head->start_add);
    // printf("size of head:%d\n",sizeof(head));
    // printf("size of structure is :%d\n",sizeof(struct mainNode));
    p = head->start_add;
    printf("%d\n",p);
}

/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
*/
void mems_finish() {
    // Implement the function according to the requirements
}

/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available.
*/

struct subNode* createNewSubNode(struct subNode* temp_sub,struct mainNode* mNode,size_t size){
    struct subNode* newNode = (struct subNode*)p;
    p = (void*)((char*)p + sizeof(struct subNode));
    newNode->main_chain_node = mNode;
    newNode->prev = temp_sub->prev;
    temp_sub->prev = newNode;
    newNode->next = temp_sub;
    newNode->status = 1;
    printf("%d\n",temp_sub->start_virt);
    void* t = temp_sub->start_virt;
    newNode->start_virt = t;
    printf("new Node start virtual address : %d\n",newNode->start_virt);
    newNode->end_virt = (void *)((char*)newNode->start_virt + size - 1);
    temp_sub->start_virt = (void*)((char*)newNode->end_virt + 1);
    return newNode;

};

struct subNode* createHole(struct mainNode* Node){
    struct subNode* hole = (struct subNode*)p;
        hole->main_chain_node = Node;
        hole->prev = NULL;
        hole->next = NULL;
        hole->status = 0;
        hole->start_virt = Node->start_virt_add;
        hole->end_virt = Node->end_virt_add;
        return hole;
};

void* mems_malloc(size_t size) {
    // Implement the function according to the requirements
    int tot_pages = 0;
    if (size%4096 == 0){
        tot_pages = (size/4096);
    }else{
        tot_pages = (size/4096) + 1;
    }
    // printf("%d\n",tot_pages);

    if(count == 0){
        struct mainNode* Node = (struct mainNode*)p;
        head->next = Node;
        Node->prev = NULL;
        Node->next = NULL;
        Node->start_virt_add = (void*)(char*)0;
        Node->end_virt_add = (void*)((char *)Node->start_virt_add + (tot_pages*4096) - 1);
        int sizeMain = sizeof(struct mainNode);
        printf("Node1 start v address:%d\n",Node->start_virt_add);
        printf("Node1 end v address:%d\n",Node->end_virt_add);
        p = (void *)((char*)p + sizeMain);
        Node->pages = tot_pages;
        printf("%d\n",p);

        //stores physical address of allocated memory
        Node->start_add = mmap(NULL, (int)tot_pages * PAGE_SIZE , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (Node->start_add == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        struct subNode* hole = createHole(Node);
        Node->subn = hole;
        printf("BigHole start and end v address:%d %d\n",Node->subn->start_virt,Node->subn->end_virt);
        // int sizeSub = sizeof(struct subNode);
        // printf("size of subNodes are:%d\n",sizeSub);
        p = (void *)((char*)p + sizeof(struct subNode));
        printf("%d\n",p);
        count ++;                               // increase count of main nodes

        // add similar function for allocating process from hole
        struct mainNode* temp = head;
        struct subNode* temp_sub = NULL;
        while(temp!=NULL){
            temp_sub = temp->subn;
            printf("BigHole start and end v address:%d %d\n",temp->subn->start_virt,temp->subn->end_virt);
            while(temp_sub!=NULL){
                if(temp_sub->status == 0 && ((char*)temp_sub->end_virt - (char*)temp_sub->start_virt +1)>=size){
                    struct subNode* sub1 = createNewSubNode(temp_sub,temp,size);
                    return sub1->start_virt;
                }
                temp_sub = temp_sub->next;
            }
            temp = temp->next;
        }
    }else{
        struct mainNode* temp = head;
        struct subNode* temp_sub = NULL;
        while(temp!=NULL){
            temp_sub = temp->subn;
            while(temp_sub!=NULL){
                if(temp_sub->status == 0 && ((char*)temp_sub->end_virt - (char*)temp_sub->start_virt +1)>=size){
                    struct subNode* Node2 = createNewSubNode(temp_sub,temp,size);
                    printf("Yes it's me!!\n");
                    return Node2->start_virt;
                }
                temp_sub = temp_sub->next;
            }
            temp = temp->next;
        }
    }
    printf("There we goo!\n");
    struct mainNode* Node = (struct mainNode*)p;
    p = (void*)((char*)p + sizeof(struct mainNode));
    struct mainNode* temp = head;
    while(temp->next!=NULL){
        temp = temp->next;
    }
    Node->prev = temp;
    temp->next = Node;
    Node->start_virt_add = (void*)((char*)temp->end_virt_add + 1);
    Node->end_virt_add = (void*)((char*)Node->start_virt_add + tot_pages*PAGE_SIZE - 1);
    Node->pages  = tot_pages;
    Node->start_add = mmap(NULL, (int)tot_pages * PAGE_SIZE , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (Node->start_add == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
    }

    struct subNode* hole = createHole(Node);
    Node->subn = hole;
    int sizeSub = sizeof(struct subNode);
    printf("size of subNodes are:%d\n",sizeSub);
    p = (void *)((char*)p + sizeSub);
    printf("%d\n",p);
    count ++;                               // increase count of main nodes

    temp = head;
    struct subNode* temp_sub = NULL;
    while(temp!=NULL){
        temp_sub = temp->subn;
        while(temp_sub!=NULL){
            if(temp_sub->status == 0 && ((char*)temp_sub->end_virt - (char*)temp_sub->start_virt +1)>=size){
                struct subNode* Node2 = createNewSubNode(temp_sub,temp,size);
                printf("Hello \n");
                return Node2->start_virt;
            }
            temp_sub = temp_sub->next;
        }
        temp = temp->next;
    }
    return NULL; // Update the return value
}

/*
Prints the stats of the MeMS system.
*/
void mems_print_stats() {
    // Implement the function according to the requirements
}

/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
*/
void* mems_get(void* v_ptr) {
    // Implement the function according to the requirements
    





    return NULL; // Update the return value
}

/*
Frees up the memory pointed by our virtual_address and adds it to the free list.
*/
void mems_free(void* v_ptr) {
    // Implement the function according to the requirements
}

int main() {
    mems_init();
    int* ptr[10];

    /*
    This allocates 10 arrays of 250 integers each
    */
    printf("\n------- Allocated virtual addresses [mems_malloc] -------\n");
    for(int i=0;i<10;i++){
        ptr[i] = (int*)mems_malloc(sizeof(int)*250);
        printf("Virtual address: %lu\n", (unsigned long)ptr[i]);
    }
    printf("%d\n",count);
    return 0;
}