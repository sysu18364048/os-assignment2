#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TLB_SIZE 16				   	//TLB��16����Ŀ 
#define PAGE_SIZE 256			   	//ҳ���СΪ256 
#define FRAME_SIZE 256			   	//֡��Ϊ256 
#define PHYSICAL_MEMORY_SIZE 65536 	//�����ڴ�Ϊ65536�ֽ� 

//ȫ�ֱ��� 
int logicalAddress = 0;				//�߼���ַ 
int offsetNumber = 0;				//ƫ���� 
int pageNumber = 0;					//ҳ�� 
int physicalAddress = 0;			//�����ַ 
int Frame = 0;						//֡�� 
int Value = 0;						//���� 
int Hit = 0;						//TLB���к����ڼ�¼֡�� 
int tlbIndex = 0;					//TLB���� 
int tlbSize = 0;					//TLB��С 
unsigned pageNumberMask = 65280;   	//ҳ�����룬65280�൱�ڶ�������1111111100000000
unsigned offsetMask = 255;         	//ƫ�����룬255�൱�ڶ�������0000000011111111
int tlbHitCount = 0;				//TLB������ 
float tlbHitRate = 0;				//TLB������ 
int addressCount = 0;				//��ַ�� 
int pageFaultCount = 0;				//ȱҳ�� 
float pageFaultRate = 0;			//ȱҳ�� 

struct tlbTable {					//TLB�ṹ�� 
    unsigned int pageNum;
    unsigned int frameNum;
};

int main(int argc, char* argv[]) {
	//����Ƿ��ѽ�BACKING_STORE.bin��addresses.txt����main���� 
    if (argc != 3) {  
        printf("û�д���BACKING_STORE.bin��addresses.txt");
        exit(1);
    }
    FILE* BACKINGSTORE = fopen(argv[1], "rb"); 					//���ļ�BACKING_STORE.bin
    FILE* addresses = fopen(argv[2], "r");						//���ļ�addresses.txt
    FILE* Output = fopen("out.txt", "w");  						//�½��ļ�out.txt�����ڴ洢������ 
    //���������ڴ����顢�������顢���� 
    int physicalMemory[PHYSICAL_MEMORY_SIZE];
    char Buffer[256];
    int Index;
    //��������ʼ��ҳ������ 
    int pageTable[PAGE_SIZE];
    memset(pageTable, -1, 256 * sizeof(int));
    //��������ʼ��TLB�ṹ�� 
    struct tlbTable tlb[TLB_SIZE];
    memset(pageTable, -1, 16 * sizeof(char));
    
    //��ʼ��addresses.txt��ѭ����ȡ���� 
    while (fscanf(addresses,"%d",&logicalAddress) == 1) {		//ȡ��һ���߼���ַ 
        addressCount++;											//��ַ����1 
        // ����λ�����λ�ƶ���ȡҳ���ƫ���� 
        pageNumber = logicalAddress & pageNumberMask;  			//��λ�����㣬�߼���ַ��8λ������Ϊҳ�룬��8λ���� 
        pageNumber = pageNumber >> 8;				   			//����8λ��ȡ��ȷ��ҳ�� 
        offsetNumber = logicalAddress & offsetMask;	   			//��λ�����㣬�߼���ַ��8λ������Ϊƫ��������8λ����
        Hit = -1;

        // ����TLB��Ѱ�����Ӧ��ҳ��
        for (Index = 0; Index < tlbSize; Index++) 
            if (tlb[Index].pageNum == pageNumber) {          	//TLB���ҵ���Ӧҳ�� 
                Hit = tlb[Index].frameNum;					 	//��ȡ֡�� 
                physicalAddress = Hit * 256 + offsetNumber;  	//���������ַ 
        }
		//�������Hit�ѱ����£�TLB���У���������1 
        if (Hit != -1) tlbHitCount++; 
        // ���TLBδ���У��ʹ�ҳ���ȡ�����ַ����ȡFIFO���ԣ���TLBͳһ���ԣ�ʵ��ҳ���û� 
        else if (pageTable[pageNumber] == -1) {  				//�����ҳ��Ӧ֡���ֵΪ-1��˵������ȱҳ�����������ҳ 
            fseek(BACKINGSTORE, pageNumber * 256, SEEK_SET);  	//����ȱҳ��ҳ�룬���ļ�ָ��ָ��󱸴洢�ļ���Ӧ��λ�� 
            fread(Buffer, sizeof(char), 256, BACKINGSTORE);	 	//�Ӻ󱸴洢�ļ��е�ҳ����¼�ڻ����� 
            pageTable[pageNumber] = Frame;						//����Ӧ֡��洢��ҳ���� 

            for (Index = 0; Index < 256; Index++) 				//�������е����ݼ��ص������ڴ�
                physicalMemory[Frame * 256 + Index] = Buffer[Index];	
            pageFaultCount++;									//ȱҳ����1 
            Frame++;											//�����ڴ�֡���1 ��ָ����һ������֡ 

            // TLBʹ��FIFO���� 
            if (tlbSize == 16) tlbSize--;						//��TLB������TLB��С��1����β��Ŀ���Ƴ����� 
            for (tlbIndex = tlbSize; tlbIndex > 0; tlbIndex--) {	//TLB��������Ŀ����
                tlb[tlbIndex].pageNum = tlb[tlbIndex - 1].pageNum;
                tlb[tlbIndex].frameNum = tlb[tlbIndex - 1].frameNum;
            }
            if (tlbSize <= 15) tlbSize++;						//��TLBδ�� ��TLB��С��1����Ŀǰ��Ŀ����TLB���� 
            tlb[0].pageNum = pageNumber;						//ҳ�������� 
            tlb[0].frameNum = pageTable[pageNumber];			//֡�������� 
            physicalAddress = pageTable[pageNumber] * 256 + offsetNumber;	//ȡ�������ַ����ɵ�ҳ 
        }
        else    			//û��ȱҳ�����������ҳ��ֱ�ӻ�ȡ֡�벢���������ַ 
            physicalAddress = pageTable[pageNumber] * 256 + offsetNumber;

        // ��ַת����ɣ��������ڴ��Ӧ��ַ��ȡ���� 
        Value = physicalMemory[physicalAddress];
        // �������ַ�������ַ�Ͷ�Ӧ���ݵ�ֵ���浽out.txt�� 
        fprintf(Output, "Virtual Address: %d Physical Address: %d Value: %d \n",
            logicalAddress, physicalAddress, Value);
    }
    //����ȱҳ�ʺ�TLB�����ʲ���ӡ��out.txt�ļ�
    pageFaultRate = pageFaultCount * 1.0f / addressCount;
    tlbHitRate = tlbHitCount * 1.0f / addressCount;
    fprintf(Output, "Page-fault rate: %f\n", pageFaultRate);		 
    fprintf(Output, "TLB hit rate %f\n", tlbHitRate);				
    //�ر������ļ���������� 
    fclose(addresses);
    fclose(BACKINGSTORE);
    fclose(Output);
    return 0;
}
