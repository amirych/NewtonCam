#include <iostream>
#include<thread>
#include<chrono>

#include "atmcdLXd.h"

using namespace std;

#define CALL_API(API_FUNC,...) { \
    err = API_FUNC(__VA_ARGS__); \
    if ( err != DRV_SUCCESS ) throw err; \
}


int main(int argc, char* argv[])
{

    if ( argc < 2 ) {
        cerr << "Usage: cam_caps path_to_detector.ini\n";
        return 1;
    }

    unsigned int err;
    int numADchans, numAmp, numPreAmpGains, BitDepth, numHSpeeds, isPreAmp;
    float HSpeed,gain;

    AndorCapabilities caps;
    caps.ulSize = sizeof(AndorCapabilities);

    try {
        CALL_API(Initialize,argv[1]);

        this_thread::sleep_for(chrono::seconds(2));

        CALL_API(GetNumberADChannels,&numADchans);

        cout << "Number of AD channrls: " << numADchans << endl;

        CALL_API(GetNumberAmp,&numAmp);

        cout << "Number of amplifiers: " << numAmp << endl;

        CALL_API(GetNumberPreAmpGains,&numPreAmpGains);

        cout << "Number of gains: " << numPreAmpGains << endl;

        for ( long i_chan = 0; i_chan < (numADchans-1); ++i_chan ) {
            cout << "   AD channel: " << i_chan << endl;

            CALL_API(GetBitDepth,i_chan,&BitDepth);
            cout << "       bit depth: " << BitDepth << endl;

            for ( long i_amp = 0; i_amp < (numAmp-1); ++i_amp ) {
                CALL_API(GetNumberHSSpeeds,i_chan,i_amp,&numHSpeeds);
                cout << "       number of HSpeeds: " << numHSpeeds << endl;

                for ( long i_speed = 0; i_speed < (numHSpeeds-1); ++i_speed ) {
                    CALL_API(GetHSSpeed,i_chan,i_amp,i_speed,&HSpeed);
                    cout << "           speed: " << HSpeed << " usecs\n";

                    for ( long i_gain = 0; i_gain < (numPreAmpGains-1); ++i_gain ) {
                        CALL_API(GetPreAmpGain,i_gain,&gain);
                        cout << "           gain: " << gain;
                        CALL_API(IsPreAmpGainAvailable,i_chan,i_amp,i_speed,i_gain,&isPreAmp);
                        if ( isPreAmp ) cout << " (available)"; else cout << " (not available)\n";
                    }
                }
            }

        }

        CALL_API(GetCapabilities,&caps);

        cout << "\n\n";
        cout << "Overlap mode: " << (caps.ulAcqModes & AC_ACQMODE_OVERLAP) << endl;

        cout << "16bit mode: " << (caps.ulPixelMode & AC_PIXELMODE_16BIT) << endl;
        cout << "32bit mode: " << (caps.ulPixelMode & AC_PIXELMODE_32BIT) << endl;

        cout << "Can set HSpeed: " << (caps.ulSetFunctions & AC_SETFUNCTION_HREADOUT) << endl;
        cout << "Can set VSpeed: " << (caps.ulSetFunctions & AC_SETFUNCTION_VREADOUT) << endl;
        cout << "Can set Temperature: " << (caps.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE) << endl;
        cout << "Can set Baseline: " << (caps.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET) << endl;
        cout << "Can set PreAmpGain: " << (caps.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN) << endl;
        cout << "Can set CropMode: " << (caps.ulSetFunctions & AC_SETFUNCTION_CROPMODE) << endl;
        cout << "Can set SpoolThread: " << (caps.ulSetFunctions & AC_SETFUNCTION_SPOOLTHREADCOUNT) << endl;
        cout << "Can set Prescans: " << (caps.ulSetFunctions & AC_SETFUNCTION_PRESCANS) << endl;

        cout << "Can read Temperature: " << (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURE) << endl;
        cout << "Can read TemperatureRange: " << (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE) << endl;
        cout << "Can read DetectorSize: " << (caps.ulGetFunctions & AC_GETFUNCTION_DETECTORSIZE) << endl;

        cout << "Polling: " << (caps.ulFeatures & AC_FEATURES_POLLING) << endl;
        cout << "Shutter: " << (caps.ulFeatures & AC_FEATURES_SHUTTER) << endl;
        cout << "ShutterEx: " << (caps.ulFeatures & AC_FEATURES_SHUTTEREX) << endl;
        cout << "Fan control: " << (caps.ulFeatures & AC_FEATURES_FANCONTROL) << endl;
        cout << "Fan control (low): " << (caps.ulFeatures & AC_FEATURES_MIDFANCONTROL) << endl;
        cout << "Can read Temp during acq.: " << (caps.ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION) << endl;
        cout << "Metadata: " << (caps.ulFeatures & AC_FEATURES_METADATA) << endl;



    } catch (unsigned int err_code) {
        cerr << "Andor API return error: " << err_code << endl;
        cerr << "Exit!" << endl;
        ShutDown();
        return err;
    }

    ShutDown();
    return 0;
}

