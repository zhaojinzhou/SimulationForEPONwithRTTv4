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

const double expectPacketLength=64;//包长的期望值
const double expectPacketLengthl=0;//半区间
const int ONU=16;//ONU总数

const double upLoadV=1000000000/8;//上传速率
const double expectPacketTime=1/0.5*expectPacketLength/(upLoadV/16);//包到来的期望时间
const double timeForSimulation=20;

const int cycleTributeNum=100;//统计cycle分布时，分割成的总格数
const double cycleTributeIncrease=0.000005;//每一格的增量

const int delayTributeNum=200;//统计delay分布时，分割成的总格数
const double delayTributeIncrease=0.000005;//每一格的增量

const int idolTributeNum=2000;//统计idol分布时，分割成的总格数
const double idolTributeIncrease=0.00000002;//每一格的增量

const double bufferSize=7820;
const double bufferSizeInTime=bufferSize/upLoadV;

class link
{
private:
	struct node{
		double timeBegin;//上传开始的时间点
		double timeEnd;//上传结束的时间点，不包含report
		double lastRoundEnd;//上一轮该ONU结束发送的时间点，即发送report的时间点
		double timePacket;//包到达的时间点
		double packetLengthInTime;//包长除以速率
		node *nextONU;//指向下一个ONU上传时间点
		node *nextPacket;//指向buffer中下一个包
		node(){timeBegin=0;timeEnd=0;timePacket=0;nextONU=NULL;nextPacket=NULL;packetLengthInTime=0;lastRoundEnd=0;}
		~node(){}
	};
	long totalPacket;//成功发出去的总包数
	double totalPacketLengthInTime;//成功发出去的包的总长度，根据速率转成了以时间为单位
	long totalPacketLose;//丢掉的总包数
	double totalPacketLoseLengthInTime;//丢掉的包的总长度，根据速率转成了以时间为单位
	long totalRound;//总的轮训轮数
	double totalDelay;//总延时
	node *lastONU;//指向已分配好时间的最后一个ONU的时间点
	node *presentONU;//当前正在处理的ONU上传时间点
	node *head;
	double totalCycleTime;
	long cycleTribute[cycleTributeNum];//统计cycle分布
	long delayTribute[delayTributeNum];//统计delay分布
	long idolTribute[idolTributeNum];//统计idol分布
	long totalIdol;//出现idol的次数
	double totalIdolTime;//出现idol的总时间
public:
	link();
	~link();
	double newPacketTime();//随机出下个包到来要过多久
	double newPacketLength();//随机出下个包的长度
	void startSimulation();//仿真开始的入口
	void makeNewPacket(node *tmp2,node *tmp1);//在新的ONU上传点生成于该点可上传的数据包，和来不及上传的一个数据包,参数为对应的ONU发送点，那个发送点要发出的第一个包
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
			totalIdol++;//又出现了一次idol
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
		makeNewPacket(tmp2,tmp1);//参数1为对应的ONU发送点，参数2为那个发送点要发出的第一个包
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
	fout1<<"共传送"<<totalPacket<<"个数据包"<<'\n';
	fout1<<"平均延时为"<<totalDelay/totalPacket<<'\n';
	fout1<<"平均cycle为"<<totalCycleTime/totalRound<<'\n';
	fout1<<totalRound/ONU;
	fout1.close();

/*
	ofstream fout2("cycleTribute.out");//每个ONU的cycle都算一次，因此是实际cycle数的ONU倍
	for (tem1=0;tem1<cycleTributeNum-1;tem1++)
		fout2<<RTTTime+cycleTributeIncrease*tem1<<"至"<<RTTTime+cycleTributeIncrease*(tem1+1)<<"有"<<cycleTribute[tem1]<<"个cycyle"<<'\n';
	fout2<<RTTTime+cycleTributeIncrease*tem1<<"以上有"<<cycleTribute[tem1]<<"个cycyle"<<'\n';
	fout2.close();
*/
/*
	ofstream fout3("delayTribute.out");
	for (tem1=0;tem1<delayTributeNum-1;tem1++)
		fout3<<RTTTime+delayTributeIncrease*tem1<<"至"<<RTTTime+delayTributeIncrease*(tem1+1)<<"有"<<delayTribute[tem1]<<"个delay"<<'\n';
	fout3<<RTTTime+delayTributeIncrease*tem1<<"以上有"<<delayTribute[tem1]<<"个delay"<<'\n';
	fout3.close();*/

/*
	ofstream fout4("idol.out");
	for (tem1=0;tem1<idolTributeNum-1;tem1++)
		fout4<<idolTributeIncrease*tem1<<"至"<<idolTributeIncrease*(tem1+1)<<"有"<<idolTribute[tem1]<<"个idol"<<'\n';
	fout4<<idolTributeIncrease*tem1<<"以上有"<<idolTribute[tem1]<<"个idol"<<'\n';
	fout4<<"共出现idol"<<totalIdol<<"次"<<'\n';
	fout4<<"平均idol为"<<totalIdolTime/totalIdol<<'\n';
	fout4<<"idol期望为"<<totalIdolTime/totalRound<<'\n';
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