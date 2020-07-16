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
//필요한 경우 헤더 파일과 함수를 추가할 수 있음

// 과제 설명서대로 구현하는 방식은 각자 다를 수 있지만 약간의 제약을 둡니다.
// 레코드 파일이 페이지 단위로 저장 관리되기 때문에 사용자 프로그램에서 레코드 파일로부터 데이터를 읽고 쓸 때도
// 페이지 단위를 사용합니다. 따라서 아래의 두 함수가 필요합니다.
// 1. readPage(): 주어진 페이지 번호의 페이지 데이터를 프로그램 상으로 읽어와서 pagebuf에 저장한다
// 2. writePage(): 프로그램 상의 pagebuf의 데이터를 주어진 페이지 번호에 저장한다
// 레코드 파일에서 기존의 레코드를 읽거나 새로운 레코드를 쓰거나 삭제 레코드를 수정할 때나
// 모든 I/O는 위의 두 함수를 먼저 호출해야 합니다. 즉 페이지 단위로 읽거나 써야 합니다.

//
// 페이지 번호에 해당하는 페이지를 주어진 페이지 버퍼에 읽어서 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp,PAGE_SIZE*pagenum,SEEK_SET);
	fread((void*)pagebuf,PAGE_SIZE,1,fp);
	return;
}

//
// 페이지 버퍼의 데이터를 주어진 페이지 번호에 해당하는 위치에 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE*pagenum, SEEK_SET);
	fwrite((void*)pagebuf, PAGE_SIZE, 1, fp);
	return;
}

//
// 새로운 레코드를 저장할 때 터미널로부터 입력받은 정보를 Person 구조체에 먼저 저장하고, pack() 함수를 사용하여
// 레코드 파일에 저장할 레코드 형태를 recordbuf에 만든다. 그런 후 이 레코드를 저장할 페이지를 readPage()를 통해 프로그램 상에
// 읽어 온 후 pagebuf에 recordbuf에 저장되어 있는 레코드를 저장한다. 그 다음 writePage() 호출하여 pagebuf를 해당 페이지 번호에
// 저장한다. pack() 함수에서 readPage()와 writePage()를 호출하는 것이 아니라 pack()을 호출하는 측에서 pack() 함수 호출 후
// readPage()와 writePage()를 차례로 호출하여 레코드 쓰기를 완성한다는 의미이다.
// 
void pack(char *recordbuf, const Person *p)
{
	sprintf(recordbuf,"%s#%s#%s#%s#%s#%s#",p->sn,p->name,p->age,p->addr,p->phone,p->email);
	return;
}

// 
// 아래의 unpack() 함수는 recordbuf에 저장되어 있는 레코드를 구조체로 변환할 때 사용한다. 이 함수가 언제 호출되는지는
// 위에서 설명한 pack()의 시나리오를 참조하면 된다.
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
// 새로운 레코드를 저장하는 기능을 수행하며, 터미널로부터 입력받은 필드값을 구조체에 저장한 후 아래의 insert() 함수를 호출한다.
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

	pack(recordBuf,p);//구조체를 buf로 변환

	if((headerBuf[8]==-1)&&(headerBuf[12]==-1))
	{//delete record no exist
		pagenum=headerBuf[0]-1;
		total_record=headerBuf[4];
		nownum=total_record - (pagenum-1)*NUM;//현재페이지에 레코드가 몇개까지 쓰여있는지 확인
		if(nownum==NUM)//꽉차있을 경우
		{
			pagenum++;
			strcpy(pageBuf,recordBuf);
			writePage(fp,pageBuf,pagenum);
			total_record++;
		}
		else//남은 공간이 있을경우
		{
			readPage(fp,pageBuf,pagenum);
			strcpy(pageBuf+100*nownum,recordBuf);
			writePage(fp,pageBuf,pagenum);
			total_record++;
		}
		pagenum=pagenum+1;
		//insert값 넣어주기
		//headpage 값 갱신
		memcpy(&headerBuf[0],&pagenum,sizeof(int));
		memcpy(&headerBuf[4],&total_record,sizeof(int));
		writePage(fp,headerBuf,0);
	}
	else{//delete record exist
		pagenum=headerBuf[8];
		recordnum=headerBuf[12];
		total_record=headerBuf[4];
		
		readPage(fp,pageBuf,pagenum);//삭제레코드 페이지 읽기

		memcpy(&headerBuf[8],&pageBuf[100*recordnum+1],sizeof(int));//다음 삭제 페이지 헤더에 갱신
		memcpy(&headerBuf[12],&pageBuf[100*recordnum+5],sizeof(int));//다음 삭제 레코드 헤더에 갱신

		writePage(fp,headerBuf,0);

		//새로운 레코드 삽입
		strcpy(pageBuf+100*recordnum,recordBuf);
		writePage(fp,pageBuf,pagenum);
	}

}

//
// 주민번호와 일치하는 레코드를 찾아서 삭제하는 기능을 수행한다.
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
	if(i>=pagenum)//입력한 학번이 존재하지 않을때
	{
		fprintf(stderr,"this sn doesn't exist.\n");
		exit(1);
	}
	//해당 레코드에 <delete mark><직전 삭제 레코드 삭제된 페이지번호><직전 삭제 레코드 번호>
	//헤더에 쓰여진 페이지,레코드 수 읽기
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

	//header page 값 변경
	memcpy(&headerBuf[8],&realpage,sizeof(int));
	memcpy(&headerBuf[12],&realrecord,sizeof(int));
	writePage(fp,headerBuf,0);

	return;
}

int main(int argc, char *argv[])
{
	FILE *fp;  // 레코드 파일의 파일 포인터
	memset(pageBuf,0xFF,PAGE_SIZE);
	memset(headerBuf,0xFF,PAGE_SIZE);
	if(argc<4){
		fprintf(stderr,"a.out <option><record file name><field values list> ");
		exit(1);
	}
	if(access(argv[2],F_OK)<0)//파일이 존재하지 않을때
	{//아예 handler page를 초기화
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
	else //파일이 존재할때. 원래있던 데이터를 불러옴
	{
		if((fp=fopen(argv[2],"r+"))<0)
		{
			fprintf(stderr,"file open error for %s\n",argv[2]);
			exit(1);
		}
		readPage(fp,headerBuf,0);//headerbuf에 담아놓는다

	}

	if(!strcmp(argv[1],"i")){//insert일 경우
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

	else if(!strcmp(argv[1],"d")){//delete일 경우
		delete(fp,argv[3]);
	}

	return 1;
}
