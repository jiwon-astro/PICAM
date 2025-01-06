#include <string>
#include <iostream>
#include <stdlib.h>
#include "..\..\Includes\picam.h"

using namespace std;

const int Nrow=1300;
const int Ncol=1340;

piflt PixelVal[Nrow][Ncol]={};

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

void DataOutput(PicamHandle camera, int frame_No){
    FILE* fp;
    char FileName[]="../../Data/frame0.txt";
    FileName[16]+=frame_No;
    fp = fopen(FileName,"w");
    //Header
    piflt temperature, exptime;
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
void Configure( PicamHandle camera, float Exptime)
{
    PicamError error;

    // - set gain
    cout << "Set analog gain: ";
    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcAnalogGain,
            PicamAdcAnalogGain_Low ); // PicamAdcAnalogGain_(High,Medium,Low)
    PrintError( error );

    // - set exposure time (in millseconds)
    cout << "Set "<<Exptime<<" ms exposure time: ";
    error = 
        Picam_SetParameterFloatingPointValue(
            camera,
            PicamParameter_ExposureTime,
            Exptime ); //milisecond 단위의 Exposure time 설정
    PrintError( error );
    
    /* => Parameter is not readable.
    piflt exptime;
    error = Picam_ReadParameterFloatingPointValue(
            camera,
            PicamParameter_ExposureTime,
            &exptime ); 
    cout << "read exposure time: ";
    PrintError( error );
    if(error==PicamError_None)
        cout<< "Exposure time(ms) : "<<exptime<<endl;
    */
    // - show that the modified parameters need to be applied to hardware
    pibln committed; //True or False(Boolean)
    Picam_AreParametersCommitted( camera, &committed );
    if( committed )
        cout << "Parameters have not changed" << endl;
    else
        cout << "Parameters have been modified" << endl;

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
    }
}

void Exposure( PicamHandle camera, const PicamAvailableData& available )
{
    /*
    PicamPixelFormat format; // Monochrome16Bit or Monochrome32Bit - PIXIS1300은 16bit.
    //Picam_GetParameterIntegerValue()라는 함수는 PicamHandle 객체 camera에서 특정 parameter를 받아와서 value에다가 저장.(& 기호)
    Picam_GetParameterIntegerValue(
        camera,
        PicamParameter_PixelFormat,
        reinterpret_cast<piint*>( &format ) );

    piint bit_depth;
    Picam_GetParameterIntegerValue(
        camera,
        PicamParameter_PixelBitDepth, //PixelBitDepth : Reports the size of a data pixel in bits-per-pixel.
        &bit_depth ); 
     
    //cout << "bit depth " << bit_depth << "bits" << endl;
    */
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
    
    cout << "Num Pixels : " << pixel_count << endl; //for PIXIS1300, 1340X1300
    //cout << "size of pi16u : " << sizeof(pi16u) << endl; // 2 (16bit==2Byte)
    /*
    //Creating Dynamic Array
    piflt** PixelVal = (piflt**)malloc(sizeof(piflt) * Nrow);
    for(int i=0;i<Nrow;i++)
    {
        PixelVal[i]= (piflt*)malloc(sizeof(piflt) * Ncol);
    }
    */
    //Pixel Readout Process
    for( piint r = 0; r < available.readout_count; ++r )
    {
        const pi16u* pixel = reinterpret_cast<const pi16u*>(
            static_cast<const pibyte*>( available.initial_readout ) +
            r*readout_stride ); //pixel이라는 것은 데이터의 initial readout의 pointer(첫 픽셀의 pointer)를 의미. 따라서 모든 pixel들의 값을 읽어오려면 그다음 주소로 넘어가야한다.
        cout << "Acquireing" << endl;
        
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
        DataOutput(camera, (int)r);
        /*
        for(int i=Nrow;i>=0;--i){
            for(int j=Ncol;j>=0;--j){
                cout<<PixelVal[i][j]<<" ";
            }
            cout<<endl;
        }
        */
        /*
       int cnt=1;
        piflt mean = 0.0; // Mean Value를 출력하는 Part.
        for( piint p = 0; p < pixel_count; ++p ){
            if(p%Ncol==0){
                cout << cnt++ <<" : "<< *pixel << endl;
            }
            mean += *pixel++;
        }
        mean /= pixel_count;
        cout << "    Mean Intensity: " << mean << endl;
        */
    }
    cout<<"end process"<<endl;

    /*
    //Release memory 
    for(int i=0;i<Nrow;i++){
        free(PixelVal[i]);
    }
    free(PixelVal);
    */
}

// - Rs some data and displays the mean intensity
void Acquire( PicamHandle camera )
{
    std::cout << "Acquire data: ";

    // - acquire some data
    const pi64s readout_count = 1;
    const piint readout_time_out = -1;  // infinite
    PicamAvailableData available;
    PicamAcquisitionErrorsMask errors;
    PicamError error =
        Picam_Acquire(
            camera,
            readout_count,
            readout_time_out,
            &available,
            &errors );
    PrintError( error );

    // - print results
    if( error == PicamError_None && errors == PicamAcquisitionErrorsMask_None )
        Exposure( camera, available ); // Exposure.
    else
    {
        if( error != PicamError_None )
        {
            std::cout << "    Acquisition failed (";
            PrintEnumString( PicamEnumeratedType_Error, error );
            std::cout << ")" << std::endl;
        }

        if( errors != PicamAcquisitionErrorsMask_None )
        {
            std::cout << "    The following acquisition errors occurred: ";
            PrintEnumString(
                PicamEnumeratedType_AcquisitionErrorsMask,
                errors );
            std::cout << std::endl;
        }
    }
}

int main(){
    
    float Exptime=500.0; //Exposure time(ms)
    int Num_frame=5;

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
    
    cout <<"Exposure Time(ms) : "; 
    scanf("%f",&Exptime);

    Configure( camera, Exptime);
    Acquire( camera ); // Exposure camera
    cout << endl;

    // - allow the optional argument 'lock' to wait for temperature lock
    pibool lock = false;
    //char lock_flag;
    //3. Temperature Check====================================================
    cout << "Temperature" << endl
              << "===========" << endl;    
    ReadTemperature( camera, lock );
    cout << endl;

    //Exposure==============================================================
    //cout <<"Number of Frames : "; 
    //scanf("%d",&Num_frame);


    Picam_CloseCamera( camera );

    Picam_UninitializeLibrary();
    system("pause");
}