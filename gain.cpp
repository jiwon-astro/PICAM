#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include  <windows.h>
#include <math.h>
#include "..\Includes\picam.h"

using namespace std;

const int Nrow=1300;
const int Ncol=1340;

piflt PixelVal[Nrow][Ncol]={};

//Setting Test Params.
float SetTemp=-55;
float ti=0.0,tf=1.0,dt=0.1;
int Nset, Nbias;

//char quality[]="Low Noise";
//char gain[]="Low";

string quality;
string gain;


// - prints any picam enum
void PrintEnumString( PicamEnumeratedType type, piint value )
{
    const pichar* string;
    Picam_GetEnumerationString( type, value, &string );
    cout << string;
    Picam_DestroyString( string );
}

// - prints the camera identity
void PrintCameraID( const PicamCameraID& id )
{
    // - print the model
    PrintEnumString( PicamEnumeratedType_Model, id.model );

    // - print the serial number and sensor
    cout << " (SN:" << id.serial_number << ")"
              << " ["    << id.sensor_name   << "]" << endl;
}

// - prints error code(이상이 있는지 없는지 Check)
void PrintError( PicamError error )
{
    if( error == PicamError_None )
        cout << "Succeeded" << std::endl;
    else
    {
        cout << "Failed (";
        PrintEnumString( PicamEnumeratedType_Error, error );
        cout << ")" << endl;
    }
}

void ElapsedTime(clock_t start){
    clock_t now;
    double time, s=0;
    int h=0, m=0;
    //Get time
    now=clock();
    time=(double)(now-start)/CLOCKS_PER_SEC;
    
    //Time Conversion
    m = (int)time/60;
    h = m/60;
    s = fmodf(time, 60);
    m = m % 60;

    printf("Elapsed time : %02d h %02d m %0.2f s ",h,m,s) ;
}

void DataOutput(PicamHandle camera, int frame_No, int cycle_No, int flag, float exptime){
    FILE* fp;
    char FileName[]="../Data/frame0000_00_b.txt";

    for(int k=0, tmp;k<3;k++){
        if(frame_No<0) break;
        else{
            tmp=frame_No%10;
            FileName[16-k]+=tmp;
            frame_No/=10;
        }
    }
    for(int k=0, tmp; k<2;k++){
        if(cycle_No<0) break;
        else{
            tmp=cycle_No%10;
            FileName[19-k]+=tmp;
            cycle_No/=10;
        }
    }
    if(flag==1) FileName[21]='l';

    fp = fopen(FileName,"w");
    //Header => fitsio library.
    piflt temperature;
   
    fprintf(fp,"Exposure time(ms) : %f \n",exptime);
    fprintf(fp,"Quality : %s \n",quality.c_str());
    fprintf(fp,"Gain : %s \n",gain.c_str());

    Picam_ReadParameterFloatingPointValue(
            camera,
            PicamParameter_SensorTemperatureReading,
            &temperature );
    fprintf(fp,"Temperature(degC) : %f \n",temperature);

    //Data
    for(int i=0;i<Nrow;i++){
        for(int j=0;j<Ncol;j++){
            fprintf(fp,"%f,",PixelVal[i][j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
}

void QualitySetting(PicamHandle camera){
    piflt qualityVal;
    PicamError error;

    if(quality.compare("LN")==0){
        qualityVal=PicamAdcQuality_LowNoise;
        quality="Low Noise";
    }
    else{
        qualityVal=PicamAdcQuality_HighCapacity;
        quality="High Capacity";
    }
    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcQuality,
            qualityVal); // PicamAdcQuality_(LowNoise, HighlyCapacity)
    PrintError( error );
}

void GainSetting(PicamHandle camera){
    piflt gainVal;
    PicamError error;

    if(gain.compare("Low")==0) gainVal=PicamAdcAnalogGain_Low;
    else if(gain.compare("Medium")==0) gainVal=PicamAdcAnalogGain_Medium;
    else gainVal=PicamAdcAnalogGain_High;

    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcAnalogGain,
            gainVal ); // PicamAdcAnalogGain_(High,Medium,Low)
    PrintError( error );
}

// - changes some common camera parameters and applies them to hardware
void Configure( PicamHandle camera)
{
    PicamError error;

    // - set quality
    cout << "Set image quality: ";
    QualitySetting(camera);

    // - set gain
    cout << "Set analog gain: ";
    GainSetting(camera);

    // - set temperature point
    cout << "Set temperature point: ";
    error=Picam_SetParameterFloatingPointValue(
            camera,
            PicamParameter_SensorTemperatureSetPoint,
            SetTemp); 
    PrintError(error);

    // - apply the changes to hardware
    cout << "Commit to hardware: ";
    const PicamParameter* failed_parameters;
    piint failed_parameters_count;
    error =
        Picam_CommitParameters(
            camera,
            &failed_parameters,
            &failed_parameters_count );
    PrintError( error );

    // - show that the modified parameters need to be applied to hardware
    pibln committed; //True or False(Boolean)
    Picam_AreParametersCommitted( camera, &committed );
    if( committed ) // 이거 로직이 반대로 아닌가?????????????
        cout << "Parameters have been modified" << endl;
    else
        cout << "Parameters have not changed" << endl;

    // - print any invalid parameters
    if( failed_parameters_count > 0 )
    {
        cout << "The following parameters are invalid:" << endl;
        for( piint i = 0; i < failed_parameters_count; ++i )
        {
            cout << "    ";
            PrintEnumString(
                PicamEnumeratedType_Parameter,
                failed_parameters[i] );
            cout << endl;
        }
    }

    // - free picam-allocated resources
    Picam_DestroyParameters( failed_parameters );
}

// - reads the temperature and temperature status directly from hardware
//   and waits for temperature to lock if requested
void ReadTemperature( PicamHandle camera, pibool lock )
{
    PicamError error;

    // - read temperature
    cout << "Read sensor temperature: ";
    piflt temperature;
    error =
        Picam_ReadParameterFloatingPointValue(
            camera,
            PicamParameter_SensorTemperatureReading,
            &temperature );
    PrintError( error );
    if( error == PicamError_None )
    {
        cout << "    " << "Temperature is "
                  << temperature << " degrees C" << endl;
    }

    // - read temperature status
    cout << "Read sensor temperature status: ";
    PicamSensorTemperatureStatus status;
    error =
        Picam_ReadParameterIntegerValue(
            camera,
            PicamParameter_SensorTemperatureStatus,
            reinterpret_cast<piint*>( &status ) );
    PrintError( error );
    if( error == PicamError_None )
    {
        cout << "    " << "Status is ";
        PrintEnumString( PicamEnumeratedType_SensorTemperatureStatus, status );
        cout << endl;
    }

    // - wait indefinitely for temperature to lock if requested
    if( lock )
    {
        cout << "Waiting for temperature lock: ";
        error =
            Picam_WaitForStatusParameterValue(
                camera,
                PicamParameter_SensorTemperatureStatus,
                PicamSensorTemperatureStatus_Locked,
                -1 );
        PrintError( error );
         if( error == PicamError_None )
        {
            cout << "    " << "Temperature is "
                  << temperature << " degrees C" << endl;
        }

    }
}

void MonitorTemperature( PicamHandle camera, float SetTemp)
{
    PicamError error;

    // - read temperature
    cout << "Read sensor temperature: ";
    piflt temperature;
    error =
        Picam_ReadParameterFloatingPointValue(
            camera,
            PicamParameter_SensorTemperatureReading,
            &temperature );
    PrintError( error );
    if( error == PicamError_None )
    {
        cout << "    " << "Temperature is "
                  << temperature << " degrees C" << endl;
    }

    // - read temperature status
    cout << "Read sensor temperature status: ";
    PicamSensorTemperatureStatus status;
    error =
        Picam_ReadParameterIntegerValue(
            camera,
            PicamParameter_SensorTemperatureStatus,
            reinterpret_cast<piint*>( &status ) );
    PrintError( error );
    if( error == PicamError_None )
    {
        cout << "    " << "Status is ";
        PrintEnumString( PicamEnumeratedType_SensorTemperatureStatus, status );
        cout << endl;
    }

    //read temperature 
    clock_t start = clock();
    cout << "------------------" <<endl;
    while(temperature!=SetTemp){
        error =
            Picam_ReadParameterFloatingPointValue(
                camera,
                PicamParameter_SensorTemperatureReading,
                &temperature );
        if( error == PicamError_None )
        {   
            ElapsedTime(start); 
            cout << "  |  T = "<< temperature << " deg C" << endl;
            Sleep(5000);
        }
    }

    //Temperature lock check.
    cout << "temperature lock: ";
    error=Picam_WaitForStatusParameterValue(
            camera,
            PicamParameter_SensorTemperatureStatus,
            PicamSensorTemperatureStatus_Locked,
            -1 );
    PrintError(error);   

    //Temperature stabilization
    cout << "Waiting for stabilization : ";
    piflt tmp, avgT=0;
    while(avgT!=SetTemp){
        Sleep(10000);
        avgT=0;
        for(int i=0;i<5;i++){
            Picam_ReadParameterFloatingPointValue(
                    camera,
                    PicamParameter_SensorTemperatureReading,
                    &tmp );
            avgT+=tmp;
            Sleep(5000);
        }
        avgT/=5;
    }
    cout << "stabilized" <<endl;
}

void Exposure( PicamHandle camera, const PicamAvailableData& available )
{
    PicamError error;
    
    piint readout_stride; //다음 readout으로 이동하는데 필요한 길이를 Byte단위로

    Picam_GetParameterIntegerValue(
        camera,
        PicamParameter_ReadoutStride,
        &readout_stride );
    
    //cout << "readout : " << readout_stride << endl; 

    piint frame_size; // Camera Frame Size
    Picam_GetParameterIntegerValue(
        camera,
        PicamParameter_FrameSize,
        &frame_size ); 

    //cout << "Frame Size : " << frame_size << endl;

    //Number of Pixels
    const piint pixel_count = frame_size / sizeof( pi16u ); //sizeof 함수는 pi16u(unsigned 16bit int)가 메모리 공간을 몇 Byte차지하고 있는가를 return
    
    //cout << "Num Pixels : " << pixel_count << endl; //for PIXIS1300, 1340X1300

    //Pixel Readout Process
    for( piint r = 0; r < available.readout_count; ++r )
    {
        const pi16u* pixel = reinterpret_cast<const pi16u*>(
            static_cast<const pibyte*>( available.initial_readout ) +
            r*readout_stride ); //pixel이라는 것은 데이터의 initial readout의 pointer(첫 픽셀의 pointer)를 의미. 따라서 모든 pixel들의 값을 읽어오려면 그다음 주소로 넘어가야한다.
        
        piflt values;
        piflt mean = 0.0; // Mean Value를 출력하는 Part.
        //piflt bin_values=0; // Binning
        for(int i=0;i<Nrow;i++){
            for(int j=0;j<Ncol;j++){
                values=*pixel++;
                PixelVal[i][j]=values;
                mean+=values;
            }
            //cout<<endl;
        }   
        mean /= pixel_count;
        cout << "    Mean Intensity: " << mean << endl;
    }
}

int GetData(PicamHandle camera, PicamError error1,int frame_No, int cycle_No, int flag, float expt)
{
    const pi64s readout_count = 1;
    const piint readout_time_out = -1;  // infinite
    int break_flag=0;
    PicamAvailableData available;
    PicamAcquisitionErrorsMask errors;
    PicamError error2;
    //Acquire
    error2 = Picam_Acquire(
            camera,
            readout_count,
            readout_time_out,
            &available,
            &errors );

    // - print results
    if( error1 == PicamError_None && error2 == PicamError_None && errors==PicamAcquisitionErrorsMask_None)
    {
        cout << "Acquiring frame "<< cycle_No;
        if(flag==0) cout<<" ( Bias frame )";
        else cout<<"( Light frame )"; 
        cout<<" / Exposure Time(ms) : "<< expt; 
        Exposure( camera, available); // Exposure.
        DataOutput(camera, frame_No, cycle_No, flag, expt);
    }
    else
    {
        if( error1 != PicamError_None )
        {
            std::cout << "    Configuration Exposure Time failed (";
            PrintEnumString( PicamEnumeratedType_Error, error1 );
            cout << ")" << endl;
            break_flag=1;
        }
        if( error2 != PicamError_None )
        {
            std::cout << "    Acquisition failed (";
            PrintEnumString( PicamEnumeratedType_Error, error2 );
            cout << ")" << endl;
            break_flag=1;
        }
        if(errors != PicamAcquisitionErrorsMask_None)
        {
            cout << "    The following acquisition errors occurred: ";
            PrintEnumString(
                PicamEnumeratedType_AcquisitionErrorsMask,
                errors );
            cout << endl;
            break_flag=1;
        }
    }
    return break_flag;
}
// - Rs some data and displays the mean intensity
void Acquire( PicamHandle camera, char bias_flag)
{
    // - acquire some data
    PicamAvailableData available;
    PicamAcquisitionErrorsMask errors;
    PicamError error1, error2;
    /*
    cout<<'Acquire check process :';
    PicamError error =Picam_Acquire(
                camera,
                readout_count,
                readout_time_out,
                &available,
                &errors );
    PrintError( error );
    */
    int cnt=0; 
    int break_flag;
    float tqueue[2]={};
    for(float t=ti;t<=tf;t+=dt){
         cout << "frame No :"<< cnt <<endl;
        tqueue[1]=t; //Bias(t=0) and light frame(t)
        for(int j=0;j<2;j++){
            cout <<"--------------------"<<endl;
            // - set exposure time (in millseconds)
            error1=Picam_SetParameterFloatingPointValue(
                camera,
                PicamParameter_ExposureTime,
                tqueue[j]); //milisecond 단위의 Exposure time 설정
            
            cout<<"set exposure time : ";
            PrintError(error1);

            const PicamParameter* failed_parameters;
            piint failed_parameters_count;
            error1 =
                Picam_CommitParameters(
                    camera,
                    &failed_parameters,
                    &failed_parameters_count );
            cout<<"commit : ";
            PrintError( error1 );

            if(j==0){
                if(bias_flag=='Y'){ //Bias Frame
                    for(int i=0;i<Nbias;i++){
                        break_flag=GetData(camera,error1,cnt,0,j,tqueue[0]);
                        if(break_flag==1) break;
                    }
                }
                else continue;
            }
            else{
                for(int i=0;i<Nset;i++){ // Image Set iteration 
                    break_flag=GetData(camera,error1,cnt,i,j,tqueue[1]);
                    if(break_flag==1) break;
                }  
            }
            cout<<"end process"<<endl;
        }
        cout<<endl;
        cnt++;
    }
}

int main(){

    Picam_InitializeLibrary(); //initialize PICam Library

    PicamHandle camera;
    PicamCameraID id;
    char bias_flag;

    //1.Opening Camera=====================================================
    if( Picam_OpenFirstCamera( &camera ) == PicamError_None )
    {
        Picam_GetCameraID(camera, &id );
    }
    else // Demo Camera(예외처리)
    {
        Picam_ConnectDemoCamera(
            PicamModel_Pixis100B,
            "12345",
            &id );
        Picam_OpenCamera( &id, &camera );
    }

    PrintCameraID( id );
    cout << endl;
    //2. Configuration========================================================
    cout << "Configuration" << endl
              << "=============" << endl;
    
    cout <<"Sensor Tempaerature Set Point(-61~25degC): "; 
    cin >> SetTemp;    
    cout << endl;

    cout <<"Image Quality(Low Noise(LN) / High Capacity(HC)): "; 
    cin >> quality;    
    cout << endl;

    cout <<"Gain(Low/Medium/High): "; 
    cin >> gain;    
    cout << endl;

    cout<<"Do you want to Acquire bias frame?[Y/N] :";
    cin >> bias_flag;
    if(bias_flag=='Y'){
        cout <<"Number of bias frame : ";
        scanf("%d",&Nbias);
    }
    cout<<endl;

    cout <<"Number of frame sets(Light-Bias) : ";
    scanf("%d",&Nset);
    cout << endl;

    Configure(camera);
    cout << endl;

    
    //3. Temperature Check====================================================
    cout << "Temperature" << endl
              << "===========" << endl; 
    // - allow the optional argument 'lock' to wait for temperature lock
    //pibool lock = true;   
    //ReadTemperature( camera, lock );
    MonitorTemperature(camera,SetTemp);
    cout << endl;
    
    //Exposure setting=======================================================
    cout <<"Exposure Time setting" << endl
        << "=========================="<< endl
        <<"Initial(ti) : "; 
    scanf("%f",&ti);
    cout << "Final(tf) : ";
    scanf("%f",&tf);
    cout <<"dt : ";
    scanf("%f",&dt);
    cout << endl;
    
    //Ntime=(tf-ti)/dt+1;

    //Acquire data=======================================================
    cout << "Acquiring data" << endl
              << "===========" << endl; 
    Acquire( camera, bias_flag); // Exposure camera

    Picam_CloseCamera( camera );

    Picam_UninitializeLibrary();
    system("pause");
}
