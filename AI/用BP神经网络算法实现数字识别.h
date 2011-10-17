//来源：http://tieba.baidu.com/f?kz=192058012

******************************************************************************/     
#include <stdlib.h>     
#include <stdio.h>     
#include <math.h>     
#include <conio.h>     
   
/******************************************************************************    
                宏定义                     
******************************************************************************/     
typedef int          BOOL;      
typedef int          INT;     
typedef double        REAL;     
typedef char          CHAR;     
   
#define FALSE        0     
#define TRUE          1     
#define NOT          !     
#define AND          &&     
#define OR            ||     
   
#define MIN(x,y)      ((x)<(y) ? (x) : (y))     
#define MAX(x,y)      ((x)>(y) ? (x) : (y))     
#define sqr(x)        ((x)*(x))     
   
#define LO            0.1     
#define HI            0.9     
#define BIAS          0.5     
   
#define NUM_LAYERS    3      //网络层数     
#define NUM_DATA      10      //样本数     
#define X            6      //每个样本的列数     
#define Y            7      //每个样本的行数     
   
#define N            (X * Y) //输入层神经元个数     
#define M            10      //输出层神经元个数     
/////////////////////////////////////////////////////////////////////////////////////////     
//                              结构变量声明                                              //     
/////////////////////////////////////////////////////////////////////////////////////////     
typedef struct {                        
        INT          Units;        //层神经元数量     
        REAL*        Output;        //输出数 （即输出个矢量元素个数）     
        REAL*        Activation;    //激活值     
        REAL*        Error;        //本层误差                
        REAL**        Weight;        //连接权     
        REAL**        WeightSave;    //保存训练调整后的连接权     
        REAL**        dWeight;      //调整量     
} LAYER;    //神经网络层结构     
   
typedef struct {                        
        LAYER**      Layer;        //神经网络各层指针     
        LAYER*        InputLayer;    //输入层     
        LAYER*        OutputLayer;  //输出层     
        REAL          Alpha;        //冲量参数     
        REAL          Eta;          //学习率            
        REAL          Error;        //总误差     
        REAL          Epsilon;      //控制精度     
} NET; //  神经网络     
   
INT                  Units[NUM_LAYERS] = {N, 10, M}; //用一维数组记录各层神经元个数     
   
FILE*                f;//声明文件指针     
   
REAL                  Input[NUM_DATA][N];//用来记录学习样本输入模式     
REAL                  Inputtest[NUM_DATA][N];//用来记录测试样本输入模式     
   
/******************************************************************************    
                  各函数声明    
******************************************************************************/     
void InitializeRandoms(); //设置伪随机数种子     
INT  RandomEqualINT(INT Low, INT High);//产生一个LOW - TOP之间的伪随机整数     
REAL RandomEqualREAL(REAL Low, REAL High);//产生一个LOW - TOP之间的伪随机浮点数 

void FinalizeApplication(NET* Net);//关闭文件     
void RandomWeights(NET* Net) ;//随机生成网络各层联接权     
void SaveWeights(NET* Net);//保存网络各层联接权，以防丢失宝贵的联接权     
void RestoreWeights(NET* Net);//恢复网络各层联接权，以便重建网络     
void GenerateNetwork(NET* Net);//创建网络，为网络分配空间     
void InitializeApplication(NET* Net);//将学习样本转换成为输入模式，并创建一个文件以保存显示结果     
void SimulateNet(NET* Net, REAL* Input, REAL* Target, BOOL Training,BOOL Protocoling);//将每个样本投入网络运作     
void SetInput(NET* Net, REAL* Input,BOOL Protocoling);// 获得输入层的输出          
void PropagateLayer(NET* Net, LAYER* Lower, LAYER* Upper);//计算当前层的网络输出，upper 为当前层，LOWER为前一层     
void PropagateNet(NET* Net);//计算整个网络各层的输出     
void GetOutput(NET* Net, REAL* Output,BOOL Protocoling);//获得输出层的输出     
void ComputeOutputError(NET* Net, REAL* Target);//计算网络输出层的输出误差     
void BackpropagateLayer(NET* Net, LAYER* Upper, LAYER* Lower);//当前层误差反向传播     
void BackpropagateNet(NET* Net);////整个网络误差的后传     
void AdjustWeights(NET* Net); //调整网络各层联接权，提取样本特征     
void WriteInput(NET* Net, REAL* Input);//显示输入模式        
void WriteOutput(NET* Net, REAL* Output);//显示输出模式      
void Initializetest();//将测试样本转换成为输入模式     
   
/******************************************************************************    
    学习样本       
******************************************************************************/     
CHAR                  Pattern[NUM_DATA][Y][X]={    {" OOO ",     
                                                    "O  O",     
                                                    "O  O",     
                                                    "O  O",     
                                                    "O  O",     
                                                    "O  O",     
                                                    " OOO "  },     
   
                                                  { "  O  ",     
                                                    " OO  ",     
                                                    "O O  ",     
                                                    "  O  ",     
                                                    "  O  ",     
                                                    "  O  ",     
                                                    "  O  "  },     
   
                                                  { " OOO ",     
                                                    "O  O",     
                                                    "    O",     
                                                    "  O ",     
                                                    "  O  ",     
                                                    " O  ",     
                                                    "OOOOO"  },     
   
                                                  { " OOO ",     
                                                    "O  O",    
                                                    "    O",     
                                                    " OOO ",     
                                                    "    O",     
                                                    "O  O",     
                                                    " OOO "  },     
   
                                                  { "  O ",     
                                                    "  OO ",     
                                                    " O O ",     
                                                    "O  O ",     
                                                    "OOOOO",     
                                                    "  O ",     
                                                    "  O "  },     
   
                                                  { "OOOOO",     
                                                    "O    ",     
                                                    "O    ",     
                                                    "OOOO ",     
                                                    "    O",     
                                                    "O  O",     
                                                    " OOO "  },     
   
                                                  { " OOO ",     
                                                    "O  O",     
                                                    "O    ",     
                                                    "OOOO ",     
                                                    "O  O",     
                                                    "O  O",     
                                                    " OOO "  },     
   
                                                  { "OOOOO",     
                                                    "    O",     
                                                    "    O",     
                                                    "  O ",     
                                                    "  O  ",     
                                                    " O  ",     
                                                    "O    "  },     
   
                                                  { " OOO ",     
                                                    "O  O",     
                                                    "O  O",     
                                                    " OOO ",     
                                                    "O  O",     
                                                    "O  O",     
                                                    " OOO "  },     
   
                                                  { " OOO ",     
                                                    "O  O",     
                                                    "O  O",     
                                                    " OOOO",     
                                                    "    O",     
                                                    "O  O",     
                                                    " OOO "  } };     
/******************************************************************************  
 测试样本  
******************************************************************************/  
CHAR testPattern[NUM_DATA][Y][X] = { {" OO ",  
 "O O",  
 "O O",  
 "O O",  
 " ",  
 "O O",  
 " OOO " },  
  
 { " O O",  
 " O O",  
 " O ",  
 " ",  
 " O ",  
 " O ",  
 " O " },  
  
 { " OOO ",  
 "O O",  
 "O O",  
 " O ",  
 " O ",  
 " O ",  
 "OOOOO" },  
  
 { " OOO ",  
 "O O",  
 " O",  
 "OOOO ",  
 " O",  
 "O O",  
 " OOO " },  
  
 { " O ",  
 " OO ",  
 " O O ",  
 "O O ",  
 " OO ",  
 " O ",  
 " O " },  
  
 { "OOOOO",  
 "O ",  
 "O ",  
 "O ",  
 " O",  
 "O O",  
 " OOO " },  
  
 { " OOO ",  
 "O O",  
 "O O",  
 "O ",  
 "O O",  
 "O O",  
 " O O " },  
  
 { " O ",  
 " OO ",  
 "O O ",  
 " O ",  
 " O ",  
 " O ",  
 " O " },  
  
 { " OOO ",  
 "O O",  
 "O O",  
 " OOO ",  
 "O O",  
 "O O",  
 " OOO " },  
  
 { " OOO ",  
 "O O O",  
 "O O O",  
 " OOOO",  
 " O",  
 "O O",  
 " OOO " } };  
/******************************************************************************  
//导师信号，按从上到下的顺序分别表示0～9  
******************************************************************************/  
  
REAL Target[NUM_DATA][M] =  
 { {HI, LO, LO, LO, LO, LO, LO, LO, LO, LO},  
 {LO, HI, LO, LO, LO, LO, LO, LO, LO, LO},  
 {LO, LO, HI, LO, LO, LO, LO, LO, LO, LO},  
 {LO, LO, LO, HI, LO, LO, LO, LO, LO, LO},  
 {LO, LO, LO, LO, HI, LO, LO, LO, LO, LO},  
 {LO, LO, LO, LO, LO, HI, LO, LO, LO, LO},  
 {LO, LO, LO, LO, LO, LO, HI, LO, LO, LO},  
 {LO, LO, LO, LO, LO, LO, LO, HI, LO, LO},  
 {LO, LO, LO, LO, LO, LO, LO, LO, HI, LO},  
 {LO, LO, LO, LO, LO, LO, LO, LO, LO, HI} };  
/******************************************************************************  
 主程序  
******************************************************************************/  
void main()  
{  
 INT m,n,count;//循环变量  
 NET Net;//网络变量声明  
 BOOL Stop;//学习是否结束的控制变量  
  
 REAL Error;//记录当前所有样本的最大误差  
 InitializeRandoms();//生成随机数  
 GenerateNetwork(&Net);//创建网络并初始化网络，分配空间  
 RandomWeights(&Net);//初始化网络联接权  
 InitializeApplication(&Net);//初始化输入层，将学习样本转换成输入模式  
  
 count=0;//显示学习进度的控制变量  
 do {  
 Error = 0;//误差  
 Stop = TRUE;//初始化  
 for (n=0; n<NUM_DATA; n++) { //对每个模式计算模拟神经网络误差  
 SimulateNet(&Net, Input[n],Target[n], FALSE, FALSE); //计算模拟神经网络误差  
 Error = MAX(Error, Net.Error);//巧妙的做法，获取结构的值,获取误差最大值  
 Stop = Stop AND (Net.Error < Net.Epsilon);  
 count++;  
 }  
 Error = MAX(Error, Net.Epsilon);//作用:防止溢出,保证到100%的时候停止训练，获取误差最大值  
 if (count%300==0){  
 printf("Training %0.0f%% completed ...\\n", (Net.Epsilon / Error) * 100);  
 }//只能做一个参考，并非单调上升的值  
 if (NOT Stop) {  
 for (m=0; m<10*NUM_DATA; m++) { //对各模式进行训练  
 n = RandomEqualINT(0,NUM_DATA-1); //随机选择训练模式  
 SimulateNet(&Net, Input[n],Target[n], TRUE,FALSE );  
 }  
 }  
 } while (NOT Stop);  
 printf("Network learning is Over!\\n Please put any key to work.\\n");  

  getch();  
 SaveWeights(&Net);//学习结束后保存宝贵的联接权  
 //网络开始工作  
 Initializetest();//初始化测试样本，将其转化成输入模式  
  
 for (n=0; n<NUM_DATA; n++) {  
 SimulateNet(&Net, Inputtest[n],Target[n], FALSE, TRUE);  
 }  
 FinalizeApplication(&Net);//关闭文件  
  
 printf("Network finish it’s work .\\nPlease check the rusult in the file:result.txt.\\n");  
 printf("Please put any key to over this program.\\n");  
 getch();  
 getch();  
}  
  
/******************************************************************************  
 产生随机数  
******************************************************************************/  
  
//设置伪随机数种子  
void InitializeRandoms()  
{  
 srand(4711);  
}  
//产生一个LOW - TOP之间的伪随机整数  
INT RandomEqualINT(INT Low, INT High)  
{  
 return rand() % (High-Low+1) + Low;  
}  
//产生一个LOW - TOP之间的伪随机浮点数  
REAL RandomEqualREAL(REAL Low, REAL High)  
{  
 return ((REAL) rand() / RAND_MAX) * (High-Low) + Low;  
}  
/******************************************************************************  
 //关闭文件  
******************************************************************************/  
void FinalizeApplication(NET* Net)  
{  
 fclose(f);  
}  
/******************************************************************************  
//随机生成联接权  
******************************************************************************/  
void RandomWeights(NET* Net)  
{  
 INT l,i,j;  
  
 for (l=1; l<NUM_LAYERS; l++) { //每层  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Net->Layer[l]->Weight[i][j] = RandomEqualREAL(-0.5, 0.5);//随机值  
 }  
 }  
 }  
}  
/******************************************************************************  
//保存连接权，防止丢失宝贵的联接权  
******************************************************************************/  
void SaveWeights(NET* Net)  
{  
 INT l,i,j;  
  
 for (l=1; l<NUM_LAYERS; l++) {  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Net->Layer[l]->WeightSave[i][j] = Net->Layer[l]->Weight[i][j];  
 }  
 }  
 }  
}  
/******************************************************************************  
//恢复连接权，以便需要的时候可以重新调用，重组网络  
******************************************************************************/  
void RestoreWeights(NET* Net)  
{  
 INT l,i,j;  
  
 for (l=1; l<NUM_LAYERS; l++) {  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Net->Layer[l]->Weight[i][j] = Net->Layer[l]->WeightSave[i][j];  
 }  
 }  
 }  
}  
/******************************************************************************  
//创建网络，为网络分配空间  
******************************************************************************/  
void GenerateNetwork(NET* Net)  
{  
 INT l,i;  
  
 Net->Layer = (LAYER**) calloc(NUM_LAYERS, sizeof(LAYER*));  
  
 for (l=0; l<NUM_LAYERS; l++) {  
 Net->Layer[l] = (LAYER*) malloc(sizeof(LAYER));  
  
 Net->Layer[l]->Units = Units[l];  
 Net->Layer[l]->Output = (REAL*) calloc(Units[l]+1, sizeof(REAL));  
 Net->Layer[l]->Activation = (REAL*) calloc(Units[l]+1, sizeof(REAL));  
 Net->Layer[l]->Error = (REAL*) calloc(Units[l]+1, sizeof(REAL));  
 Net->Layer[l]->Weight = (REAL**) calloc(Units[l]+1, sizeof(REAL*));  
 Net->Layer[l]->WeightSave = (REAL**) calloc(Units[l]+1, sizeof(REAL*));  
 Net->Layer[l]->dWeight = (REAL**) calloc(Units[l]+1, sizeof(REAL*));  
 Net->Layer[l]->Output[0] = BIAS;  
  
 if (l != 0) {  
 for (i=1; i<=Units[l]; i++) {  
 Net->Layer[l]->Weight[i] = (REAL*) calloc(Units[l-1]+1, sizeof(REAL));  
 Net->Layer[l]->WeightSave[i] = (REAL*) calloc(Units[l-1]+1, sizeof(REAL));  
 Net->Layer[l]->dWeight[i] = (REAL*) calloc(Units[l-1]+1, sizeof(REAL));  
 }  
 }  
 }  
 Net->InputLayer = Net->Layer[0];//为输入层分配指针  
 Net->OutputLayer = Net->Layer[NUM_LAYERS-1];//为输出层分配指针  
 Net->Alpha = 0.8;//冲量参数  
 Net->Eta = 0.5;//学习率  
 Net->Epsilon =0.005;//控制精度  
}  
/******************************************************************************  
将输入样本转换成为输入模式，并创建一个文件以保存显示结果  
******************************************************************************/  
void InitializeApplication(NET* Net)  
{  
 INT n, i,j;  
  
 for (n=0; n<NUM_DATA; n++) {  
 for (i=0; i<Y; i++) {  
 for (j=0; j<X; j++) {  
 if ( Pattern[n][i][j] == ’O’ )  
 Input[n][i*X+j] = HI ;  
 else Input[n][i*X+j] =LO ;  
 //NUM_DATA输入模式，输入层X*Y个神经元  
 }  
 }  
 }  
 f = fopen("result.txt", "w");  
}

/******************************************************************************  
 训练网络  
//将每个样本投入网络运作，Input是转换后的输入模式，Target为导师信号，通过布尔型  
//的Training和Ptotocoling值控制是否训练和打印输入/输出模式  
******************************************************************************/  
void SimulateNet(NET* Net, REAL* Input, REAL* Target, BOOL Training,BOOL Protocoling)  
{  
 REAL Output[M]; //用来记录输出层输出  
 SetInput(Net, Input,Protocoling);//设置输入层，获得输入层的输出  
 PropagateNet(Net);//计算网络各层的输出  
 GetOutput(Net, Output,Protocoling);//获得输出层的输出  
  
 ComputeOutputError(Net, Target);//计算输出层误差  
 if (Training) {  
 BackpropagateNet(Net);//误差反向传播  
 AdjustWeights(Net);//调整联接权  
 }  
}  
/******************************************************************************  
 获得输入层的输出  
******************************************************************************/  
void SetInput(NET* Net, REAL* Input,BOOL Protocoling)  
{  
 INT i;  
  
 for (i=1; i<=Net->InputLayer->Units; i++) {  
 Net->InputLayer->Output[i] = Input[i-1]; //输入层输入  
 }  
 if (Protocoling) {  
 WriteInput(Net, Input);//根据Protocoling值输出输入模式  
 }  
}  
/******************************************************************************  
//计算当前层的网络输出，upper 为当前层，LOWER为前一层  
******************************************************************************/  
void PropagateLayer(NET* Net, LAYER* Lower, LAYER* Upper)  
{  
 INT i,j;  
 REAL Sum;  
  
 for (i=1; i<=Upper->Units; i++) {  
 Sum = 0;  
 for (j=0; j<=Lower->Units; j++) {  
 Sum += Upper->Weight[i][j] * Lower->Output[j]; //计算本层的净输入  
 }  
 Upper->Activation[i] = Sum;//保留激活值  
 //计算本层的输出,激活函数必须是S形函数，这样才可导，这是BP网络的理论前提  
 Upper->Output[i]=1/(1+exp(-Sum));  
 }  
}  
/******************************************************************************  
 //计算整个网络各层的输出  
******************************************************************************/  
void PropagateNet(NET* Net)  
{  
 INT l;  
  
 for (l=0; l<NUM_LAYERS-1; l++) {  
 PropagateLayer(Net, Net->Layer[l], Net->Layer[l+1]);  
 }  
}  
/******************************************************************************  
//获得输出层的输出  
******************************************************************************/  
void GetOutput(NET* Net, REAL* Output,BOOL Protocoling)  
{  
 INT i;  
  
 for (i=1; i<=Net->OutputLayer->Units; i++) {  
 Output[i-1] = Net->OutputLayer->Output[i];//输出层输出  
 }  
 if (Protocoling) {  
 WriteOutput(Net, Output);//根据Protocoling值输出输出模式  
 }  
}  
/******************************************************************************  
 //计算输出层误差，* Target是导师信号  
******************************************************************************/  
void ComputeOutputError(NET* Net, REAL* Target)  
{  
 INT i;  
 REAL Err,Out;  
  
 Net->Error = 0;  
 for (i=1; i<=Net->OutputLayer->Units; i++) {  
 Out = Net->OutputLayer->Output[i];//输出层的输出  
  Err = Target[i-1]-Out;//误差计算  
 Net->OutputLayer->Error[i] = Out * (1-Out) * Err;  
 //用delta规则计算误差，因为用了可导的s形函数  
 Net->Error += 0.5 * sqr(Err);//平方差公式  
 }  
}  
/******************************************************************************  
 //误差反向传播 Upper 为前层，Lower为后层 ，层数值大的为前层  
******************************************************************************/  
void BackpropagateLayer(NET* Net, LAYER* Upper, LAYER* Lower)  
{  
 INT i,j;//循环变量  
 REAL Out, Err;  
  
 for (i=1; i<=Lower->Units; i++) {  
 Out = Lower->Output[i];//后层的输出  
 Err = 0;//用来记录隐含层输出的误差的估计值  
 for (j=1; j<=Upper->Units; j++) {  
 Err += Upper->Weight[j][i] * Upper->Error[j];  
 //误差的反馈，通过已经处理的前层的delta值和联接权去估计，有理论基础  
 }  
 Lower->Error[i] =Out * (1-Out) * Err; //delta规则  
 }  
}  
/******************************************************************************  
 //整个网络误差的后传  
******************************************************************************/  
void BackpropagateNet(NET* Net)  
{  
 INT l;//循环变量  
  
 for (l=NUM_LAYERS-1; l>1; l--) {  
 BackpropagateLayer(Net, Net->Layer[l], Net->Layer[l-1]);//对每层处理  
 }  
}  
/******************************************************************************  
 //调整网络每一层的联接权  
******************************************************************************/  
void AdjustWeights(NET* Net)  
{  
 INT l,i,j;//循环变量  
 REAL Out, Err, dWeight;  
 //记录后层的输出、当前层的输出误差、当前神经元联接权上次的调整量  
  
 for (l=1; l<NUM_LAYERS; l++) {  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Out = Net->Layer[l-1]->Output[j];//后层的输出  
 Err = Net->Layer[l]->Error[i];//当前层的输出误差  
 dWeight = Net->Layer[l]->dWeight[i][j];  
 //将本神经元联接权上次的调整量取出，初始值为0，初始化网络时赋值的  
 Net->Layer[l]->Weight[i][j] += Net->Eta * Err * Out + Net->Alpha * dWeight;  
 //Alpha为冲量参数，加快网络的收敛速度  
 Net->Layer[l]->dWeight[i][j] = Net->Eta * Err * Out;  
 //记录本次神经元联接权的调整量  
 }  
 }  
 }  
}  
/******************************************************************************  
//显示输入模式  
******************************************************************************/  
void WriteInput(NET* Net, REAL* Input)  
{  
 INT i;  
  
 for (i=0; i<N; i++) {  
 if (i%X == 0) {  
 fprintf(f, "\\n");  
 }  
 fprintf(f, "%c", (Input[i] == HI) ? ’0’ : ’ ’);  
 }  
 fprintf(f, " -> ");  
}  
/******************************************************************************  
//显示输出模式  
******************************************************************************/  
void WriteOutput(NET* Net, REAL* Output)  
{  
 INT i;//循环变量  
 INT Index;//用来记录最大输出值的下标，也就是最后识别的结果  
 REAL MaxOutput;//用来记录最大的输出值  
  
 MaxOutput=0;//初始化  
 for (i=0; i<M; i++)  
 {  
 if(MaxOutput<Output[i]){  
 MaxOutput=MAX(MaxOutput,Output[i]);//保存最大值  
 Index=i;  
 }  
 }  
  
 fprintf(f, "%i\\n", Index);//写进文件  
  
}  
/******************************************************************************  
 初始化测试样本  
******************************************************************************/  
void Initializetest()  
{  
 INT n,i,j;//循环变量  
  
 for (n=0; n<NUM_DATA; n++) {  
 for (i=0; i<Y; i++) {  
 for (j=0; j<X; j++)  
 if (testPattern[n][i][j]==’O’)  
 Inputtest[n][i*X+j] = HI;  
 else Inputtest[n][i*X+j] =LO; //NUM_DATA输入模式，输入层X*Y个神经元  
 }  
 }  
}