#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "person.h"

#define NUM PAGE_SIZE/RECORD_SIZE

char recordBuf[RECORD_SIZE];
char pageBuf[PAGE_SIZE];
char headerBuf[PAGE_SIZE];
//�ʿ��� ��� ��� ���ϰ� �Լ��� �߰��� �� ����

// ���� ������� �����ϴ� ����� ���� �ٸ� �� ������ �ణ�� ������ �Ӵϴ�.
// ���ڵ� ������ ������ ������ ���� �����Ǳ� ������ ����� ���α׷����� ���ڵ� ���Ϸκ��� �����͸� �а� �� ����
// ������ ������ ����մϴ�. ���� �Ʒ��� �� �Լ��� �ʿ��մϴ�.
// 1. readPage(): �־��� ������ ��ȣ�� ������ �����͸� ���α׷� ������ �о�ͼ� pagebuf�� �����Ѵ�
// 2. writePage(): ���α׷� ���� pagebuf�� �����͸� �־��� ������ ��ȣ�� �����Ѵ�
// ���ڵ� ���Ͽ��� ������ ���ڵ带 �аų� ���ο� ���ڵ带 ���ų� ���� ���ڵ带 ������ ����
// ��� I/O�� ���� �� �Լ��� ���� ȣ���ؾ� �մϴ�. �� ������ ������ �аų� ��� �մϴ�.

//
// ������ ��ȣ�� �ش��ϴ� �������� �־��� ������ ���ۿ� �о �����Ѵ�. ������ ���۴� �ݵ�� ������ ũ��� ��ġ�ؾ� �Ѵ�.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp,PAGE_SIZE*pagenum,SEEK_SET);
	fread((void*)pagebuf,PAGE_SIZE,1,fp);
	return;
}

//
// ������ ������ �����͸� �־��� ������ ��ȣ�� �ش��ϴ� ��ġ�� �����Ѵ�. ������ ���۴� �ݵ�� ������ ũ��� ��ġ�ؾ� �Ѵ�.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE*pagenum, SEEK_SET);
	fwrite((void*)pagebuf, PAGE_SIZE, 1, fp);
	return;
}

//
// ���ο� ���ڵ带 ������ �� �͹̳ηκ��� �Է¹��� ������ Person ����ü�� ���� �����ϰ�, pack() �Լ��� ����Ͽ�
// ���ڵ� ���Ͽ� ������ ���ڵ� ���¸� recordbuf�� �����. �׷� �� �� ���ڵ带 ������ �������� readPage()�� ���� ���α׷� ��
// �о� �� �� pagebuf�� recordbuf�� ����Ǿ� �ִ� ���ڵ带 �����Ѵ�. �� ���� writePage() ȣ���Ͽ� pagebuf�� �ش� ������ ��ȣ��
// �����Ѵ�. pack() �Լ����� readPage()�� writePage()�� ȣ���ϴ� ���� �ƴ϶� pack()�� ȣ���ϴ� ������ pack() �Լ� ȣ�� ��
// readPage()�� writePage()�� ���ʷ� ȣ���Ͽ� ���ڵ� ���⸦ �ϼ��Ѵٴ� �ǹ��̴�.
// 
void pack(char *recordbuf, const Person *p)
{
	sprintf(recordbuf,"%s#%s#%s#%s#%s#%s#",p->sn,p->name,p->age,p->addr,p->phone,p->email);
	return;
}

// 
// �Ʒ��� unpack() �Լ��� recordbuf�� ����Ǿ� �ִ� ���ڵ带 ����ü�� ��ȯ�� �� ����Ѵ�. �� �Լ��� ���� ȣ��Ǵ�����
// ������ ������ pack()�� �ó������� �����ϸ� �ȴ�.
//
void unpack(const char *recordbuf, Person *p)
{
	char str[RECORD_SIZE];
	char *sArr[6]={NULL,};
	int i=0;

	strcpy(str,recordbuf);

	char *ptr = strtok(str,"#");
	while(ptr!=NULL)
	{
		sArr[i]=ptr;
		i++;
		ptr=strtok(NULL,"#");
	}

	strcpy(p->sn,sArr[0]);
	strcpy(p->name,sArr[1]);
	strcpy(p->age,sArr[2]);
	strcpy(p->addr,sArr[3]);
	strcpy(p->phone,sArr[4]);
	strcpy(p->email,sArr[5]);
	
	return;
}

//
// ���ο� ���ڵ带 �����ϴ� ����� �����ϸ�, �͹̳ηκ��� �Է¹��� �ʵ尪�� ����ü�� ������ �� �Ʒ��� insert() �Լ��� ȣ���Ѵ�.
//
void insert(FILE *fp, const Person *p)
{
	int total_record=0;
	int pagenum;
	int recordnum;
	int nownum;
	int addnum;
	memset(recordBuf,0xFF,RECORD_SIZE);
	memset(pageBuf,0xFF,PAGE_SIZE);

	pack(recordBuf,p);//����ü�� buf�� ��ȯ

	if((headerBuf[8]==-1)&&(headerBuf[12]==-1))
	{//delete record no exist
		pagenum=headerBuf[0]-1;
		total_record=headerBuf[4];
		nownum=total_record - (pagenum-1)*NUM;//������������ ���ڵ尡 ����� �����ִ��� Ȯ��
		if(nownum==NUM)//�������� ���
		{
			pagenum++;
			strcpy(pageBuf,recordBuf);
			writePage(fp,pageBuf,pagenum);
			total_record++;
		}
		else//���� ������ �������
		{
			readPage(fp,pageBuf,pagenum);
			strcpy(pageBuf+100*nownum,recordBuf);
			writePage(fp,pageBuf,pagenum);
			total_record++;
		}
		pagenum=pagenum+1;
		//insert�� �־��ֱ�
		//headpage �� ����
		memcpy(&headerBuf[0],&pagenum,sizeof(int));
		memcpy(&headerBuf[4],&total_record,sizeof(int));
		writePage(fp,headerBuf,0);
	}
	else{//delete record exist
		pagenum=headerBuf[8];
		recordnum=headerBuf[12];
		total_record=headerBuf[4];
		
		readPage(fp,pageBuf,pagenum);//�������ڵ� ������ �б�

		memcpy(&headerBuf[8],&pageBuf[100*recordnum+1],sizeof(int));//���� ���� ������ ����� ����
		memcpy(&headerBuf[12],&pageBuf[100*recordnum+5],sizeof(int));//���� ���� ���ڵ� ����� ����

		writePage(fp,headerBuf,0);

		//���ο� ���ڵ� ����
		strcpy(pageBuf+100*recordnum,recordBuf);
		writePage(fp,pageBuf,pagenum);
	}

}

//
// �ֹι�ȣ�� ��ġ�ϴ� ���ڵ带 ã�Ƽ� �����ϴ� ����� �����Ѵ�.
//
void delete(FILE *fp, const char *sn)
{
	int i=0,j=0;
	char copy[14];
	int pagenum;
	int recordnum;
	int realpage=0,realrecord=0;
	memset(pageBuf,0,PAGE_SIZE);
	strcpy(copy,sn);
	Person p;

	pagenum=headerBuf[0];

	for(i=1;i<pagenum;i++)
	{
		readPage(fp,pageBuf,i);

		for(j=0;j<NUM;j++)
		{
			memset(recordBuf,0,RECORD_SIZE);
			strcpy(recordBuf,pageBuf+j*100);
			if(recordBuf[0]=='*')
				continue;
			if(!strcmp(recordBuf,"0xFF"))
			{
				break;
			}
			unpack(recordBuf,&p);
			if(!strcmp(p.sn,copy))
			{
				realpage=i;
				realrecord=j;
				break;
			}
		}
		if(!strcmp(p.sn,copy))
			break;
	}
	if(i>=pagenum)//�Է��� �й��� �������� ������
	{
		fprintf(stderr,"this sn doesn't exist.\n");
		exit(1);
	}
	//�ش� ���ڵ忡 <delete mark><���� ���� ���ڵ� ������ ��������ȣ><���� ���� ���ڵ� ��ȣ>
	//����� ������ ������,���ڵ� �� �б�
	int d_page;
	int d_record;
	d_page=headerBuf[8];
	d_record=headerBuf[12];
	memset(pageBuf,0,PAGE_SIZE);
	readPage(fp,pageBuf,realpage);
	
	memcpy(&pageBuf[100*realrecord],"*",sizeof(char));
	memcpy(&pageBuf[100*realrecord+1],&d_page,sizeof(int));
	memcpy(&pageBuf[100*realrecord+5],&d_record,sizeof(int));

	writePage(fp,pageBuf,realpage);

	//header page �� ����
	memcpy(&headerBuf[8],&realpage,sizeof(int));
	memcpy(&headerBuf[12],&realrecord,sizeof(int));
	writePage(fp,headerBuf,0);

	return;
}

int main(int argc, char *argv[])
{
	FILE *fp;  // ���ڵ� ������ ���� ������
	memset(pageBuf,0xFF,PAGE_SIZE);
	memset(headerBuf,0xFF,PAGE_SIZE);
	if(argc<4){
		fprintf(stderr,"a.out <option><record file name><field values list> ");
		exit(1);
	}
	if(access(argv[2],F_OK)<0)//������ �������� ������
	{//�ƿ� handler page�� �ʱ�ȭ
		if((fp=fopen(argv[2],"w+"))<0)
		{
			fprintf(stderr,"file open error for %s\n",argv[2]);
			exit(1);
		}
		int n1=1;
		int n2=0;
		int n3=-1;
		memcpy(&headerBuf[0],&n1,sizeof(int));
		memcpy(&headerBuf[4],&n2,sizeof(int));
		memcpy(&headerBuf[8],&n3,sizeof(int));
		memcpy(&headerBuf[12],&n3,sizeof(int));
		writePage(fp,headerBuf,0);
	}
	else //������ �����Ҷ�. �����ִ� �����͸� �ҷ���
	{
		if((fp=fopen(argv[2],"r+"))<0)
		{
			fprintf(stderr,"file open error for %s\n",argv[2]);
			exit(1);
		}
		readPage(fp,headerBuf,0);//headerbuf�� ��Ƴ��´�

	}

	if(!strcmp(argv[1],"i")){//insert�� ���
		if(argc<9){
			fprintf(stderr,"input more field values\n");
			exit(1);
		}
		Person person;
		strcpy(person.sn,argv[3]);
		strcpy(person.name,argv[4]);
		strcpy(person.age,argv[5]);
		strcpy(person.addr,argv[6]);
		strcpy(person.phone,argv[7]);
		strcpy(person.email,argv[8]);
		
		insert(fp,&person);
	}

	else if(!strcmp(argv[1],"d")){//delete�� ���
		delete(fp,argv[3]);
	}

	return 1;
}
