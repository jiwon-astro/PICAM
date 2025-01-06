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
float ti=0.0, tf=1000.0;
float dt=50.0;
int Ntime;

char quality[]="Low Noise";
char gain[]="Low";


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

void DataOutput(PicamHandle camera, int frame_No, int cycle_No, float exptime){
    FILE* fp;
    char FileName[]="../Data/frame0000_00.txt";

    for(int j=0, tmp;j<3;j++){
        if(frame_No<0) break;
        else{
            tmp=frame_No%10;
            FileName[16-j]+=tmp;
            frame_No/=10;
        }
    }
    for(int j=0, tmp; j<2;j++){
        if(cycle_No<0) break;
        else{
            tmp=cycle_No%10;
            FileName[19-j]+=tmp;
            cycle_No/=10;
        }
    }
    fp = fopen(FileName,"w");
    //Header => fitsio library.
    piflt temperature;
    /* Parameters not readable => 그럼 어떻게..? 
    piflt exptime;
    piint Gain;

    Picam_ReadParameterFloatingPointValue(
            camera,
            PicamParameter_ExposureTime,
            &exptime ); 
    fprintf(fp,"Exposure time(ms) : %f \n",exptime);

    Picam_ReadParameterIntegerValue(
            camera,
            PicamParameter_AdcAnalogGain,
            &Gain);
    fprintf(fp,"Gain : %f \n",Gain);
    */
    fprintf(fp,"Exposure time(sec) : %f \n",exptime);
    fprintf(fp,"Quality : %s \n",quality);
    fprintf(fp,"Gain : %s \n",gain);

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

// - changes some common camera parameters and applies them to hardware
void Configure( PicamHandle camera)
{
    PicamError error;

    // - set quality
    cout << "Set image quality: ";
    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcQuality,
            PicamAdcQuality_LowNoise ); // PicamAdcAnalogGain_(LowNoise, HighlyCapacity)
    PrintError( error );

    // - set gain
    cout << "Set analog gain: ";
    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcAnalogGain,
            PicamAdcAnalogGain_Low ); // PicamAdcAnalogGain_(High,Medium,Low)
    PrintError( error );

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
    
    float Exptime=0;

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

// - Rs some data and displays the mean intensity
void Acquire( PicamHandle camera, int Nframe )
{
    cout << "Acquire data \n";

    // - acquire some data
    const pi64s readout_count = 1;
    const piint readout_time_out = -1;  // infinite
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
    for(float t=ti;t<=tf;t+=dt){
        cout <<"--------------------"<<endl;
        // - set exposure time (in millseconds)
        error1=Picam_SetParameterFloatingPointValue(
            camera,
            PicamParameter_ExposureTime,
            t*1000); //milisecond 단위의 Exposure time 설정
        
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

        //Acquire
        for(int i=0;i<Nframe;i++){
            error2 = Picam_Acquire(
                    camera,
                    readout_count,
                    readout_time_out,
                    &available,
                    &errors );

            // - print results
            if( error1 == PicamError_None && error2 == PicamError_None && errors==PicamAcquisitionErrorsMask_None)
            {
                cout << "Acquiring frame " << cnt <<"(cycle"<< i <<") / Exposure Time(sec) : "<< t; 
                Exposure( camera, available); // Exposure.
                DataOutput(camera, cnt, i, t);
            }
            else
            {
                if( error1 != PicamError_None )
                {
                    std::cout << "    Configuration Exposure Time failed (";
                    PrintEnumString( PicamEnumeratedType_Error, error1 );
                    cout << ")" << endl;
                    break;
                }
                if( error2 != PicamError_None )
                {
                    std::cout << "    Acquisition failed (";
                    PrintEnumString( PicamEnumeratedType_Error, error2 );
                    cout << ")" << endl;
                    break;
                }
                if(errors != PicamAcquisitionErrorsMask_None)
                {
                    cout << "    The following acquisition errors occurred: ";
                    PrintEnumString(
                        PicamEnumeratedType_AcquisitionErrorsMask,
                        errors );
                    cout << endl;
                }
            }
        }
        cnt++;
    }
    cout<<"end process"<<endl;

}

int main(){
    
    int Nframe;

    Picam_InitializeLibrary(); //initialize PICam Library

    PicamHandle camera;
    PicamCameraID id;

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
    scanf("%f",&SetTemp);    

    cout <<"Number of frames : ";
    scanf("%d",&Nframe);
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
    cout <<"Exposure Time setting(sec)" << endl
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
    Acquire( camera, Nframe); // Exposure camera

    Picam_CloseCamera( camera );

    Picam_UninitializeLibrary();
    system("pause");
}
