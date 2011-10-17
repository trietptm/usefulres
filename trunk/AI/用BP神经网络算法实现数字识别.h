//��Դ��http://tieba.baidu.com/f?kz=192058012

******************************************************************************/     
#include <stdlib.h>     
#include <stdio.h>     
#include <math.h>     
#include <conio.h>     
   
/******************************************************************************    
                �궨��                     
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
   
#define NUM_LAYERS    3      //�������     
#define NUM_DATA      10      //������     
#define X            6      //ÿ������������     
#define Y            7      //ÿ������������     
   
#define N            (X * Y) //�������Ԫ����     
#define M            10      //�������Ԫ����     
/////////////////////////////////////////////////////////////////////////////////////////     
//                              �ṹ��������                                              //     
/////////////////////////////////////////////////////////////////////////////////////////     
typedef struct {                        
        INT          Units;        //����Ԫ����     
        REAL*        Output;        //����� ���������ʸ��Ԫ�ظ�����     
        REAL*        Activation;    //����ֵ     
        REAL*        Error;        //�������                
        REAL**        Weight;        //����Ȩ     
        REAL**        WeightSave;    //����ѵ�������������Ȩ     
        REAL**        dWeight;      //������     
} LAYER;    //�������ṹ     
   
typedef struct {                        
        LAYER**      Layer;        //���������ָ��     
        LAYER*        InputLayer;    //�����     
        LAYER*        OutputLayer;  //�����     
        REAL          Alpha;        //��������     
        REAL          Eta;          //ѧϰ��            
        REAL          Error;        //�����     
        REAL          Epsilon;      //���ƾ���     
} NET; //  ������     
   
INT                  Units[NUM_LAYERS] = {N, 10, M}; //��һά�����¼������Ԫ����     
   
FILE*                f;//�����ļ�ָ��     
   
REAL                  Input[NUM_DATA][N];//������¼ѧϰ��������ģʽ     
REAL                  Inputtest[NUM_DATA][N];//������¼������������ģʽ     
   
/******************************************************************************    
                  ����������    
******************************************************************************/     
void InitializeRandoms(); //����α���������     
INT  RandomEqualINT(INT Low, INT High);//����һ��LOW - TOP֮���α�������     
REAL RandomEqualREAL(REAL Low, REAL High);//����һ��LOW - TOP֮���α��������� 

void FinalizeApplication(NET* Net);//�ر��ļ�     
void RandomWeights(NET* Net) ;//������������������Ȩ     
void SaveWeights(NET* Net);//���������������Ȩ���Է���ʧ���������Ȩ     
void RestoreWeights(NET* Net);//�ָ������������Ȩ���Ա��ؽ�����     
void GenerateNetwork(NET* Net);//�������磬Ϊ�������ռ�     
void InitializeApplication(NET* Net);//��ѧϰ����ת����Ϊ����ģʽ��������һ���ļ��Ա�����ʾ���     
void SimulateNet(NET* Net, REAL* Input, REAL* Target, BOOL Training,BOOL Protocoling);//��ÿ������Ͷ����������     
void SetInput(NET* Net, REAL* Input,BOOL Protocoling);// ������������          
void PropagateLayer(NET* Net, LAYER* Lower, LAYER* Upper);//���㵱ǰ������������upper Ϊ��ǰ�㣬LOWERΪǰһ��     
void PropagateNet(NET* Net);//�������������������     
void GetOutput(NET* Net, REAL* Output,BOOL Protocoling);//������������     
void ComputeOutputError(NET* Net, REAL* Target);//��������������������     
void BackpropagateLayer(NET* Net, LAYER* Upper, LAYER* Lower);//��ǰ�����򴫲�     
void BackpropagateNet(NET* Net);////�����������ĺ�     
void AdjustWeights(NET* Net); //���������������Ȩ����ȡ��������     
void WriteInput(NET* Net, REAL* Input);//��ʾ����ģʽ        
void WriteOutput(NET* Net, REAL* Output);//��ʾ���ģʽ      
void Initializetest();//����������ת����Ϊ����ģʽ     
   
/******************************************************************************    
    ѧϰ����       
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
 ��������  
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
//��ʦ�źţ������ϵ��µ�˳��ֱ��ʾ0��9  
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
 ������  
******************************************************************************/  
void main()  
{  
 INT m,n,count;//ѭ������  
 NET Net;//�����������  
 BOOL Stop;//ѧϰ�Ƿ�����Ŀ��Ʊ���  
  
 REAL Error;//��¼��ǰ����������������  
 InitializeRandoms();//���������  
 GenerateNetwork(&Net);//�������粢��ʼ�����磬����ռ�  
 RandomWeights(&Net);//��ʼ����������Ȩ  
 InitializeApplication(&Net);//��ʼ������㣬��ѧϰ����ת��������ģʽ  
  
 count=0;//��ʾѧϰ���ȵĿ��Ʊ���  
 do {  
 Error = 0;//���  
 Stop = TRUE;//��ʼ��  
 for (n=0; n<NUM_DATA; n++) { //��ÿ��ģʽ����ģ�����������  
 SimulateNet(&Net, Input[n],Target[n], FALSE, FALSE); //����ģ�����������  
 Error = MAX(Error, Net.Error);//�������������ȡ�ṹ��ֵ,��ȡ������ֵ  
 Stop = Stop AND (Net.Error < Net.Epsilon);  
 count++;  
 }  
 Error = MAX(Error, Net.Epsilon);//����:��ֹ���,��֤��100%��ʱ��ֹͣѵ������ȡ������ֵ  
 if (count%300==0){  
 printf("Training %0.0f%% completed ...\\n", (Net.Epsilon / Error) * 100);  
 }//ֻ����һ���ο������ǵ���������ֵ  
 if (NOT Stop) {  
 for (m=0; m<10*NUM_DATA; m++) { //�Ը�ģʽ����ѵ��  
 n = RandomEqualINT(0,NUM_DATA-1); //���ѡ��ѵ��ģʽ  
 SimulateNet(&Net, Input[n],Target[n], TRUE,FALSE );  
 }  
 }  
 } while (NOT Stop);  
 printf("Network learning is Over!\\n Please put any key to work.\\n");  

  getch();  
 SaveWeights(&Net);//ѧϰ�����󱣴汦�������Ȩ  
 //���翪ʼ����  
 Initializetest();//��ʼ����������������ת��������ģʽ  
  
 for (n=0; n<NUM_DATA; n++) {  
 SimulateNet(&Net, Inputtest[n],Target[n], FALSE, TRUE);  
 }  
 FinalizeApplication(&Net);//�ر��ļ�  
  
 printf("Network finish it��s work .\\nPlease check the rusult in the file:result.txt.\\n");  
 printf("Please put any key to over this program.\\n");  
 getch();  
 getch();  
}  
  
/******************************************************************************  
 ���������  
******************************************************************************/  
  
//����α���������  
void InitializeRandoms()  
{  
 srand(4711);  
}  
//����һ��LOW - TOP֮���α�������  
INT RandomEqualINT(INT Low, INT High)  
{  
 return rand() % (High-Low+1) + Low;  
}  
//����һ��LOW - TOP֮���α���������  
REAL RandomEqualREAL(REAL Low, REAL High)  
{  
 return ((REAL) rand() / RAND_MAX) * (High-Low) + Low;  
}  
/******************************************************************************  
 //�ر��ļ�  
******************************************************************************/  
void FinalizeApplication(NET* Net)  
{  
 fclose(f);  
}  
/******************************************************************************  
//�����������Ȩ  
******************************************************************************/  
void RandomWeights(NET* Net)  
{  
 INT l,i,j;  
  
 for (l=1; l<NUM_LAYERS; l++) { //ÿ��  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Net->Layer[l]->Weight[i][j] = RandomEqualREAL(-0.5, 0.5);//���ֵ  
 }  
 }  
 }  
}  
/******************************************************************************  
//��������Ȩ����ֹ��ʧ���������Ȩ  
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
//�ָ�����Ȩ���Ա���Ҫ��ʱ��������µ��ã���������  
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
//�������磬Ϊ�������ռ�  
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
 Net->InputLayer = Net->Layer[0];//Ϊ��������ָ��  
 Net->OutputLayer = Net->Layer[NUM_LAYERS-1];//Ϊ��������ָ��  
 Net->Alpha = 0.8;//��������  
 Net->Eta = 0.5;//ѧϰ��  
 Net->Epsilon =0.005;//���ƾ���  
}  
/******************************************************************************  
����������ת����Ϊ����ģʽ��������һ���ļ��Ա�����ʾ���  
******************************************************************************/  
void InitializeApplication(NET* Net)  
{  
 INT n, i,j;  
  
 for (n=0; n<NUM_DATA; n++) {  
 for (i=0; i<Y; i++) {  
 for (j=0; j<X; j++) {  
 if ( Pattern[n][i][j] == ��O�� )  
 Input[n][i*X+j] = HI ;  
 else Input[n][i*X+j] =LO ;  
 //NUM_DATA����ģʽ�������X*Y����Ԫ  
 }  
 }  
 }  
 f = fopen("result.txt", "w");  
}

/******************************************************************************  
 ѵ������  
//��ÿ������Ͷ������������Input��ת���������ģʽ��TargetΪ��ʦ�źţ�ͨ��������  
//��Training��Ptotocolingֵ�����Ƿ�ѵ���ʹ�ӡ����/���ģʽ  
******************************************************************************/  
void SimulateNet(NET* Net, REAL* Input, REAL* Target, BOOL Training,BOOL Protocoling)  
{  
 REAL Output[M]; //������¼��������  
 SetInput(Net, Input,Protocoling);//��������㣬������������  
 PropagateNet(Net);//���������������  
 GetOutput(Net, Output,Protocoling);//������������  
  
 ComputeOutputError(Net, Target);//������������  
 if (Training) {  
 BackpropagateNet(Net);//���򴫲�  
 AdjustWeights(Net);//��������Ȩ  
 }  
}  
/******************************************************************************  
 ������������  
******************************************************************************/  
void SetInput(NET* Net, REAL* Input,BOOL Protocoling)  
{  
 INT i;  
  
 for (i=1; i<=Net->InputLayer->Units; i++) {  
 Net->InputLayer->Output[i] = Input[i-1]; //���������  
 }  
 if (Protocoling) {  
 WriteInput(Net, Input);//����Protocolingֵ�������ģʽ  
 }  
}  
/******************************************************************************  
//���㵱ǰ������������upper Ϊ��ǰ�㣬LOWERΪǰһ��  
******************************************************************************/  
void PropagateLayer(NET* Net, LAYER* Lower, LAYER* Upper)  
{  
 INT i,j;  
 REAL Sum;  
  
 for (i=1; i<=Upper->Units; i++) {  
 Sum = 0;  
 for (j=0; j<=Lower->Units; j++) {  
 Sum += Upper->Weight[i][j] * Lower->Output[j]; //���㱾��ľ�����  
 }  
 Upper->Activation[i] = Sum;//��������ֵ  
 //���㱾������,�����������S�κ����������ſɵ�������BP���������ǰ��  
 Upper->Output[i]=1/(1+exp(-Sum));  
 }  
}  
/******************************************************************************  
 //�������������������  
******************************************************************************/  
void PropagateNet(NET* Net)  
{  
 INT l;  
  
 for (l=0; l<NUM_LAYERS-1; l++) {  
 PropagateLayer(Net, Net->Layer[l], Net->Layer[l+1]);  
 }  
}  
/******************************************************************************  
//������������  
******************************************************************************/  
void GetOutput(NET* Net, REAL* Output,BOOL Protocoling)  
{  
 INT i;  
  
 for (i=1; i<=Net->OutputLayer->Units; i++) {  
 Output[i-1] = Net->OutputLayer->Output[i];//��������  
 }  
 if (Protocoling) {  
 WriteOutput(Net, Output);//����Protocolingֵ������ģʽ  
 }  
}  
/******************************************************************************  
 //�����������* Target�ǵ�ʦ�ź�  
******************************************************************************/  
void ComputeOutputError(NET* Net, REAL* Target)  
{  
 INT i;  
 REAL Err,Out;  
  
 Net->Error = 0;  
 for (i=1; i<=Net->OutputLayer->Units; i++) {  
 Out = Net->OutputLayer->Output[i];//���������  
  Err = Target[i-1]-Out;//������  
 Net->OutputLayer->Error[i] = Out * (1-Out) * Err;  
 //��delta�����������Ϊ���˿ɵ���s�κ���  
 Net->Error += 0.5 * sqr(Err);//ƽ���ʽ  
 }  
}  
/******************************************************************************  
 //���򴫲� Upper Ϊǰ�㣬LowerΪ��� ������ֵ���Ϊǰ��  
******************************************************************************/  
void BackpropagateLayer(NET* Net, LAYER* Upper, LAYER* Lower)  
{  
 INT i,j;//ѭ������  
 REAL Out, Err;  
  
 for (i=1; i<=Lower->Units; i++) {  
 Out = Lower->Output[i];//�������  
 Err = 0;//������¼��������������Ĺ���ֵ  
 for (j=1; j<=Upper->Units; j++) {  
 Err += Upper->Weight[j][i] * Upper->Error[j];  
 //���ķ�����ͨ���Ѿ������ǰ���deltaֵ������Ȩȥ���ƣ������ۻ���  
 }  
 Lower->Error[i] =Out * (1-Out) * Err; //delta����  
 }  
}  
/******************************************************************************  
 //�����������ĺ�  
******************************************************************************/  
void BackpropagateNet(NET* Net)  
{  
 INT l;//ѭ������  
  
 for (l=NUM_LAYERS-1; l>1; l--) {  
 BackpropagateLayer(Net, Net->Layer[l], Net->Layer[l-1]);//��ÿ�㴦��  
 }  
}  
/******************************************************************************  
 //��������ÿһ�������Ȩ  
******************************************************************************/  
void AdjustWeights(NET* Net)  
{  
 INT l,i,j;//ѭ������  
 REAL Out, Err, dWeight;  
 //��¼�����������ǰ����������ǰ��Ԫ����Ȩ�ϴεĵ�����  
  
 for (l=1; l<NUM_LAYERS; l++) {  
 for (i=1; i<=Net->Layer[l]->Units; i++) {  
 for (j=0; j<=Net->Layer[l-1]->Units; j++) {  
 Out = Net->Layer[l-1]->Output[j];//�������  
 Err = Net->Layer[l]->Error[i];//��ǰ���������  
 dWeight = Net->Layer[l]->dWeight[i][j];  
 //������Ԫ����Ȩ�ϴεĵ�����ȡ������ʼֵΪ0����ʼ������ʱ��ֵ��  
 Net->Layer[l]->Weight[i][j] += Net->Eta * Err * Out + Net->Alpha * dWeight;  
 //AlphaΪ�����������ӿ�����������ٶ�  
 Net->Layer[l]->dWeight[i][j] = Net->Eta * Err * Out;  
 //��¼������Ԫ����Ȩ�ĵ�����  
 }  
 }  
 }  
}  
/******************************************************************************  
//��ʾ����ģʽ  
******************************************************************************/  
void WriteInput(NET* Net, REAL* Input)  
{  
 INT i;  
  
 for (i=0; i<N; i++) {  
 if (i%X == 0) {  
 fprintf(f, "\\n");  
 }  
 fprintf(f, "%c", (Input[i] == HI) ? ��0�� : �� ��);  
 }  
 fprintf(f, " -> ");  
}  
/******************************************************************************  
//��ʾ���ģʽ  
******************************************************************************/  
void WriteOutput(NET* Net, REAL* Output)  
{  
 INT i;//ѭ������  
 INT Index;//������¼������ֵ���±꣬Ҳ�������ʶ��Ľ��  
 REAL MaxOutput;//������¼�������ֵ  
  
 MaxOutput=0;//��ʼ��  
 for (i=0; i<M; i++)  
 {  
 if(MaxOutput<Output[i]){  
 MaxOutput=MAX(MaxOutput,Output[i]);//�������ֵ  
 Index=i;  
 }  
 }  
  
 fprintf(f, "%i\\n", Index);//д���ļ�  
  
}  
/******************************************************************************  
 ��ʼ����������  
******************************************************************************/  
void Initializetest()  
{  
 INT n,i,j;//ѭ������  
  
 for (n=0; n<NUM_DATA; n++) {  
 for (i=0; i<Y; i++) {  
 for (j=0; j<X; j++)  
 if (testPattern[n][i][j]==��O��)  
 Inputtest[n][i*X+j] = HI;  
 else Inputtest[n][i*X+j] =LO; //NUM_DATA����ģʽ�������X*Y����Ԫ  
 }  
 }  
}