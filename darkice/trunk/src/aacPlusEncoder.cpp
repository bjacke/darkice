/*------------------------------------------------------------------------------

   Copyright (c) 2005 Tyrell Corporation. All rights reserved.

   Tyrell DarkIce

   File     : aacPlusEncoder.cpp
   Version  : $Revision$
   Author   : $Author$
   Location : $HeadURL$

   Copyright notice:

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 3
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

------------------------------------------------------------------------------*/

/* ============================================================ include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// compile the whole file only if aacplus support configured in
#ifdef HAVE_FDKAAC_LIB



#include "Exception.h"
#include "Util.h"
#include "aacPlusEncoder.h"


/* ===================================================  local data structures */


/* ================================================  local constants & macros */

/*------------------------------------------------------------------------------
 *  File identity
 *----------------------------------------------------------------------------*/
static const char fileid[] = "$Id$";


/* ===============================================  local function prototypes */


/* =============================================================  module code */

/*------------------------------------------------------------------------------
 *  Open an encoding session
 *----------------------------------------------------------------------------*/
bool
aacPlusEncoder :: open ( void )
{
    if ( isOpen() ) {
        close();
    }

    // open the underlying sink
    if ( !sink->open() ) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac lib opening underlying sink error");
    }

    reportEvent(1, "Using fdk-aac codec");

    int InChannels = getInChannel();
    int OutChannels = getOutChannel();

    if (aacEncOpen(&encoderHandle, 0, InChannels) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to open encoder");
		return 1;
	}

    int aot;
    int bitrate = getOutBitrate() * 1000;

    if (bitrate >= 64000)
        aot = AOT_AAC_LC;
    else
    {
        if (bitrate < 64000 && bitrate > 32000)
            aot = AOT_SBR;
        else
            aot = (InChannels == 1)? AOT_SBR : AOT_PS; // HE-AAC / HE-AAC v2
    }

    if (aacEncoder_SetParam(encoderHandle, AACENC_AOT, aot) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac Unable to set the AOT");
		return 1;
	}

    if (aacEncoder_SetParam(encoderHandle, AACENC_SAMPLERATE, getOutSampleRate()) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set the sample rate");
		return 1;
	}

    CHANNEL_MODE mode;
    switch (OutChannels) {
	case 1:
        mode = MODE_1;       break;
	case 2:
        mode = MODE_2;       break;
	case 3:
        mode = MODE_1_2;     break;
	case 4:
        mode = MODE_1_2_1;   break;
	case 5:
        mode = MODE_1_2_2;   break;
	case 6:
        mode = MODE_1_2_2_1; break;;
    }

    if (aacEncoder_SetParam(encoderHandle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set the channel mode");
		return 1;
	}

	if (aacEncoder_SetParam(encoderHandle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set the channel order");
		return 1;
	}

	if (aacEncoder_SetParam(encoderHandle, AACENC_BITRATE, bitrate) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set the bitrate");
        return 1;
    }

    if (aacEncoder_SetParam(encoderHandle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to ADTS mux mode");
		return 1;
	}
    if (aacEncEncode(encoderHandle, NULL, NULL, NULL, NULL) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to initialize");
		return 1;
	}

    if (aacEncoder_SetParam(encoderHandle, AACENC_BANDWIDTH, lowpass)) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set lowpass");
		return 1;
    }

    if (aacEncoder_SetParam(encoderHandle, AACENC_AFTERBURNER, 1) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to set afterburner");
		return 1;
    }

    AACENC_InfoStruct info = { 0 };
    if (aacEncInfo(encoderHandle, &info) != AACENC_OK) {
        throw Exception( __FILE__, __LINE__,
                         "fdk-aac unable to get encoder info initialize");
		return 1;
	}

    inputSamples = InChannels * info.frameLength;
    // initialize the resampling coverter if needed
    if ( converter ) {
#ifdef HAVE_SRC_LIB
        converterData.input_frames   = 4096/((getInBitsPerSample() / 8) * getInChannel());
        converterData.data_in        = new float[converterData.input_frames*getInChannel()];
        converterData.output_frames  = (int) (converterData.input_frames * resampleRatio + 1);
        if ((int) inputSamples >  getInChannel() * converterData.output_frames) {
            resampledOffset       = new float[2 * inputSamples];
        } else {
            resampledOffset       = new float[2 * getInChannel() * converterData.input_frames];
        }
        converterData.src_ratio      = resampleRatio;
        converterData.end_of_input   = 0;
#else
        converter->initialize( resampleRatio, getInChannel());
        //needed 2x(converted input samples) to handle offsets
    int outCount                 = 2 * getInChannel() * (inputSamples + 1);
        if (resampleRatio > 1)
        outCount = (int) (outCount * resampleRatio);
        resampledOffset = new short int[outCount];
#endif
        resampledOffsetSize = 0;
    }

    aacplusOpen = true;
    reportEvent(10, "nChannelsAAC", OutChannels);
    reportEvent(10, "sampleRateAAC", getOutSampleRate());
    reportEvent(10, "inSamples", inputSamples);
    return true;
}


/*------------------------------------------------------------------------------
 *  Write data to the encoder
 *----------------------------------------------------------------------------*/
unsigned int
aacPlusEncoder :: write (  const void    * buf,
                        unsigned int    len )
{
    if ( !isOpen() || len == 0) {
        return 0;
    }

    unsigned int    channels         = getInChannel();
    unsigned int    bitsPerSample    = getInBitsPerSample();
    unsigned int    sampleSize       = (bitsPerSample / 8) * channels;
    unsigned char * b                = (unsigned char*) buf;
    unsigned int    processed        = len - (len % sampleSize);
    unsigned int    nSamples         = processed / sampleSize;
    unsigned char * aacplusBuf          = new unsigned char[maxOutputBytes];
    int             samples          = (int) nSamples * channels;
    int             processedSamples = 0;


    if ( converter ) {
        unsigned int         converted;
#ifdef HAVE_SRC_LIB
        src_short_to_float_array ((short *) b, (float *) converterData.data_in, samples);
        converterData.input_frames   = nSamples;
        converterData.data_out = resampledOffset + (resampledOffsetSize * channels);
        int srcError = src_process (converter, &converterData);
        if (srcError)
             throw Exception (__FILE__, __LINE__, "libsamplerate error: ", src_strerror (srcError));
        converted = converterData.output_frames_gen;
#else
        int         inCount  = nSamples;
        short int     * shortBuffer  = new short int[samples];
        int         outCount = (int) (inCount * resampleRatio);
        Util::conv( bitsPerSample, b, processed, shortBuffer, isInBigEndian());
        converted = converter->resample( inCount,
                                         outCount+1,
                                         shortBuffer,
                                         &resampledOffset[resampledOffsetSize*channels]);
        delete[] shortBuffer;
#endif
        resampledOffsetSize += converted;

        AACENC_ERROR err;
        // encode samples (if enough)
        while(resampledOffsetSize - processedSamples >= inputSamples / channels) {
#ifdef HAVE_SRC_LIB
            short *shortData = new short[inputSamples];
            src_float_to_short_array(resampledOffset + (processedSamples * channels),
                                     shortData, inputSamples) ;

            if ((err = aacEncEncode(encoderHandle, &shortData, &aacplusBuf, &in_args, &out_args)) != AACENC_OK) {
                if (err == AACENC_ENCODE_EOF)
                    break;
                throw Exception( __FILE__, __LINE__,
                                 "fdk-aac Encoding failed");
                return 1;
            }

//            int outputBytes = aacplusEncEncode(encoderHandle,
//                                       (int32_t*) shortData,
//                                        inputSamples,
//                                        aacplusBuf,
//                                        maxOutputBytes);
            delete [] shortData;
#else

            int outputBytes = aacplusEncEncode(encoderHandle,
                                       (int32_t*) &resampledOffset[processedSamples*channels],
                                        inputSamples,
                                        aacplusBuf,
                                        maxOutputBytes);
#endif
            unsigned int wrote = getSink()->write(aacplusBuf, outputBytes);
            
            if (wrote < outputBytes) {
                reportEvent(3, "aacPlusEncoder :: write, couldn't write full data to underlying sink");
            }

            processedSamples+=inputSamples/channels;
        }

        if (processedSamples && (int) resampledOffsetSize >= processedSamples) {
            resampledOffsetSize -= processedSamples;
            //move least part of resampled data to beginning
            if(resampledOffsetSize)
#ifdef HAVE_SRC_LIB
                resampledOffset = (float *) memmove(resampledOffset, &resampledOffset[processedSamples*channels],
                                                    resampledOffsetSize*channels*sizeof(float));
#else
                resampledOffset = (short *) memmove(resampledOffset, &resampledOffset[processedSamples*channels],
                                                    resampledOffsetSize*sampleSize);
#endif
        }
    } else {
        while (processedSamples < samples) {
            int     inSamples = samples - processedSamples < (int) inputSamples
                              ? samples - processedSamples
                              : inputSamples;

            int outputBytes = aacplusEncEncode(encoderHandle,
                                       (int32_t*) (b + processedSamples/sampleSize),
                                        inSamples,
                                        aacplusBuf,
                                        maxOutputBytes);
            
            unsigned int wrote = getSink()->write(aacplusBuf, outputBytes);
            
            if (wrote < outputBytes) {
                reportEvent(3, "aacPlusEncoder :: write, couldn't write full data to underlying sink");
            }
            
            processedSamples += inSamples;
        }
    }

    delete[] aacplusBuf;

//    return processedSamples;
    return samples * sampleSize;
}

/*------------------------------------------------------------------------------
 *  Flush the data from the encoder
 *----------------------------------------------------------------------------*/
void
aacPlusEncoder :: flush ( void )
{
    if ( !isOpen() ) {
        return;
    }

    sink->flush();
}


/*------------------------------------------------------------------------------
 *  Close the encoding session
 *----------------------------------------------------------------------------*/
void
aacPlusEncoder :: close ( void )
{
    if ( isOpen() ) {
        flush();

        aacEncClose(&encoderHandle);
        aacplusOpen = false;
    
        sink->close();
    }
}


#endif // HAVE_AACPLUS_LIB
