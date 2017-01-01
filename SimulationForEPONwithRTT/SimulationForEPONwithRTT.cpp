//#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <time.h>
using namespace std;

const double guardTime=0.000001;
const double reportTime=0.0000001;
const double grantTime=0.0000001;
const double RTTTime=0.0002;

const double expectPacketLength=64;//����������ֵ
const double expectPacketLengthl=0;//������
const int ONU=16;//ONU����

const double upLoadV=1000000000/8;//�ϴ�����
const double expectPacketTime=1/0.5*expectPacketLength/(upLoadV/16);//������������ʱ��
const double timeForSimulation=20;

const int cycleTributeNum=100;//ͳ��cycle�ֲ�ʱ���ָ�ɵ��ܸ���
const double cycleTributeIncrease=0.000005;//ÿһ�������

const int delayTributeNum=200;//ͳ��delay�ֲ�ʱ���ָ�ɵ��ܸ���
const double delayTributeIncrease=0.000005;//ÿһ�������

const int idolTributeNum=2000;//ͳ��idol�ֲ�ʱ���ָ�ɵ��ܸ���
const double idolTributeIncrease=0.00000002;//ÿһ�������

const double bufferSize=7820;
const double bufferSizeInTime=bufferSize/upLoadV;

class link
{
private:
	struct node{
		double timeBegin;//�ϴ���ʼ��ʱ���
		double timeEnd;//�ϴ�������ʱ��㣬������report
		double lastRoundEnd;//��һ�ָ�ONU�������͵�ʱ��㣬������report��ʱ���
		double timePacket;//�������ʱ���
		double packetLengthInTime;//������������
		node *nextONU;//ָ����һ��ONU�ϴ�ʱ���
		node *nextPacket;//ָ��buffer����һ����
		node(){timeBegin=0;timeEnd=0;timePacket=0;nextONU=NULL;nextPacket=NULL;packetLengthInTime=0;lastRoundEnd=0;}
		~node(){}
	};
	long totalPacket;//�ɹ�����ȥ���ܰ���
	double totalPacketLengthInTime;//�ɹ�����ȥ�İ����ܳ��ȣ���������ת������ʱ��Ϊ��λ
	long totalPacketLose;//�������ܰ���
	double totalPacketLoseLengthInTime;//�����İ����ܳ��ȣ���������ת������ʱ��Ϊ��λ
	long totalRound;//�ܵ���ѵ����
	double totalDelay;//����ʱ
	node *lastONU;//ָ���ѷ����ʱ������һ��ONU��ʱ���
	node *presentONU;//��ǰ���ڴ����ONU�ϴ�ʱ���
	node *head;
	double totalCycleTime;
	long cycleTribute[cycleTributeNum];//ͳ��cycle�ֲ�
	long delayTribute[delayTributeNum];//ͳ��delay�ֲ�
	long idolTribute[idolTributeNum];//ͳ��idol�ֲ�
	long totalIdol;//����idol�Ĵ���
	double totalIdolTime;//����idol����ʱ��
public:
	link();
	~link();
	double newPacketTime();//������¸�������Ҫ�����
	double newPacketLength();//������¸����ĳ���
	void startSimulation();//���濪ʼ�����
	void makeNewPacket(node *tmp2,node *tmp1);//���µ�ONU�ϴ��������ڸõ���ϴ������ݰ������������ϴ���һ�����ݰ�,����Ϊ��Ӧ��ONU���͵㣬�Ǹ����͵�Ҫ�����ĵ�һ����
};

void link::makeNewPacket(node *tmp2,node *tmp1)
{
	node *tmp;
	while (tmp1->timePacket<tmp2->timeEnd)
	{
		tmp=new node;
		tmp->timePacket=tmp1->timePacket+newPacketTime();
		tmp->packetLengthInTime=newPacketLength()/upLoadV;
		tmp1->nextPacket=tmp;
		tmp1=tmp;
	}
}

void link::startSimulation()
{
	node *tmp1,*tmp2,*tmp3;
	double timetmp1;
	double delayInThisONUCycle;
	long packetNumInThisONUCycle;
	int tem1,tem2;
	ofstream fout1("cycleLength.out");
	presentONU=head->nextONU;
	while (presentONU->timeBegin<timeForSimulation)
	{
		tmp1=presentONU->nextPacket;
		delayInThisONUCycle=0;
		packetNumInThisONUCycle=0;
		
		while (tmp1->timePacket<presentONU->lastRoundEnd)
		{
			totalDelay=totalDelay+presentONU->timeEnd-tmp1->timePacket;
			delayInThisONUCycle=delayInThisONUCycle+presentONU->timeEnd-tmp1->timePacket;
			packetNumInThisONUCycle++;
			totalPacket++;

			tem1=floor((presentONU->timeEnd-tmp1->timePacket+RTTTime/2-RTTTime)/delayTributeIncrease);
			if (tem1>=delayTributeNum-1)
				delayTribute[delayTributeNum-1]++;
			else
				delayTribute[tem1]++;

			if (tmp1->nextPacket!=NULL)
				tmp1=tmp1->nextPacket;
			else
				break;
		}
		totalRound++;
		tmp2=new node;
		tmp2->nextPacket=tmp1;
		lastONU->nextONU=tmp2;
		if ((presentONU->timeEnd+reportTime+grantTime+RTTTime)>(lastONU->timeEnd+reportTime+guardTime))
		{
			tmp2->timeBegin=presentONU->timeEnd+reportTime+grantTime+RTTTime;
			totalIdol++;//�ֳ�����һ��idol
			totalIdolTime=totalIdolTime+(presentONU->timeEnd+reportTime+grantTime+RTTTime)-(lastONU->timeEnd+reportTime+guardTime);
			tem1=floor(((presentONU->timeEnd+reportTime+grantTime+RTTTime)-(lastONU->timeEnd+reportTime+guardTime))/idolTributeIncrease);
			if (tem1>=idolTributeNum-1)
				idolTribute[idolTributeNum-1]++;
			else
				idolTribute[tem1]++;
		}
		else
		{
			tmp2->timeBegin=lastONU->timeEnd+reportTime+guardTime;
		}
		if (totalRound % ONU==1)
			fout1<<"cycle:"<<tmp2->timeBegin-presentONU->timeBegin<<" "<<delayInThisONUCycle/packetNumInThisONUCycle<<'\n';
		totalCycleTime=totalCycleTime+tmp2->timeBegin-presentONU->timeBegin;
		//tmp2->timeEnd=tmp2->timeBegin+timetmp1;
		
		tem1=floor((tmp2->timeBegin-presentONU->timeBegin-RTTTime)/cycleTributeIncrease);
		if (tem1>=cycleTributeNum-1)
			cycleTribute[cycleTributeNum-1]++;
		else
			cycleTribute[tem1]++;

		timetmp1=0;
		tmp1=tmp2->nextPacket;
		while (tmp1->timePacket<presentONU->timeEnd)
		{
			timetmp1=timetmp1+tmp1->packetLengthInTime;
			if (tmp1->nextPacket!=NULL)
				tmp1=tmp1->nextPacket;
			else
				break;
		}
		tmp2->lastRoundEnd=presentONU->timeEnd;
		tmp2->timeEnd=tmp2->timeBegin+timetmp1;
		//tmp2->nextPacket=tmp1;
		makeNewPacket(tmp2,tmp1);//����1Ϊ��Ӧ��ONU���͵㣬����2Ϊ�Ǹ����͵�Ҫ�����ĵ�һ����
		lastONU=tmp2;
		tmp2=presentONU;
		presentONU=presentONU->nextONU;
		head->nextONU=presentONU;
		
		tmp1=tmp2->nextPacket;
		while (tmp1!=lastONU->nextPacket)
		{
			tmp3=tmp1->nextPacket;
			delete tmp1;
			tmp1=tmp3;
		}
		delete tmp2;
	}
	fout1<<"������"<<totalPacket<<"�����ݰ�"<<'\n';
	fout1<<"ƽ����ʱΪ"<<totalDelay/totalPacket<<'\n';
	fout1<<"ƽ��cycleΪ"<<totalCycleTime/totalRound<<'\n';
	fout1<<totalRound/ONU;
	fout1.close();

/*
	ofstream fout2("cycleTribute.out");//ÿ��ONU��cycle����һ�Σ������ʵ��cycle����ONU��
	for (tem1=0;tem1<cycleTributeNum-1;tem1++)
		fout2<<RTTTime+cycleTributeIncrease*tem1<<"��"<<RTTTime+cycleTributeIncrease*(tem1+1)<<"��"<<cycleTribute[tem1]<<"��cycyle"<<'\n';
	fout2<<RTTTime+cycleTributeIncrease*tem1<<"������"<<cycleTribute[tem1]<<"��cycyle"<<'\n';
	fout2.close();
*/
/*
	ofstream fout3("delayTribute.out");
	for (tem1=0;tem1<delayTributeNum-1;tem1++)
		fout3<<RTTTime+delayTributeIncrease*tem1<<"��"<<RTTTime+delayTributeIncrease*(tem1+1)<<"��"<<delayTribute[tem1]<<"��delay"<<'\n';
	fout3<<RTTTime+delayTributeIncrease*tem1<<"������"<<delayTribute[tem1]<<"��delay"<<'\n';
	fout3.close();*/

/*
	ofstream fout4("idol.out");
	for (tem1=0;tem1<idolTributeNum-1;tem1++)
		fout4<<idolTributeIncrease*tem1<<"��"<<idolTributeIncrease*(tem1+1)<<"��"<<idolTribute[tem1]<<"��idol"<<'\n';
	fout4<<idolTributeIncrease*tem1<<"������"<<idolTribute[tem1]<<"��idol"<<'\n';
	fout4<<"������idol"<<totalIdol<<"��"<<'\n';
	fout4<<"ƽ��idolΪ"<<totalIdolTime/totalIdol<<'\n';
	fout4<<"idol����Ϊ"<<totalIdolTime/totalRound<<'\n';
	fout4.close();*/
}

double link::newPacketTime()
{
	double i;
	i=(double)((rand()<<15)+rand())/((RAND_MAX<<15)+RAND_MAX);
	i=expectPacketTime*log(1/(1-i));
	return(i);
}

double link::newPacketLength()
{
	//return(double(rand())/32767.0*expectPacketLengthl*2+expectPacketLength-expectPacketLengthl);
	return(expectPacketLength);
}

link::link()
{
	int i;
	node *tmp1,*tmp2,*tmp4;
	double tmp3;
	head=new node;
	totalCycleTime=0;
	presentONU=head;
	totalRound=0;
	totalPacketLose=0;
	totalPacket=0;
	totalIdolTime=0;
	totalPacketLengthInTime=0;
	totalPacketLoseLengthInTime=0;
	totalDelay=0;
	totalIdol=0;
	for (i=0;i<=idolTributeNum-1;i++)
		idolTribute[i]=0;
	for (i=0;i<=cycleTributeNum-1;i++)
		cycleTribute[i]=0;
	for (i=0;i<=delayTributeNum-1;i++)
		delayTribute[i]=0;
	for (i=1;i<=ONU;i++)
	{
		tmp1=new node;
		presentONU->nextONU=tmp1;
		tmp1->timeBegin=presentONU->timeEnd+guardTime+grantTime;
		tmp1->timeEnd=tmp1->timeBegin+expectPacketLength/upLoadV;
		tmp2=new node;
		tmp1->nextPacket=tmp2;
		tmp2->timePacket=(tmp1->timeEnd-tmp1->timeBegin)/2+tmp1->timeBegin;
		tmp2->packetLengthInTime=expectPacketLength/upLoadV;
		tmp4=new node;
		tmp2->nextPacket=tmp4;
		tmp4->timePacket=tmp1->timeEnd+reportTime;
		tmp4->packetLengthInTime=expectPacketLength/upLoadV;
		//tmp1->lastRoundEnd=tmp3;
		presentONU=tmp1;
	}
	lastONU=presentONU;
	
}

link::~link()
{
	node *tmp1,*tmp2;
	while (presentONU!=NULL)
	{
		tmp1=presentONU->nextPacket;
		while (tmp1!=NULL)
		{
			tmp2=tmp1;
			tmp1=tmp1->nextPacket;
			delete tmp2;
		}
		tmp2=presentONU;
		presentONU=presentONU->nextONU;
		delete tmp2;
	}
	delete head;
}

int main()
{
	double k;
	srand((unsigned)time(NULL));
	link newlink;
	newlink.startSimulation();
	//cin>>k;
	return 0;
}