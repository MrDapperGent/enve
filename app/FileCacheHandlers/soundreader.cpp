#include "soundreader.h"
#include "audiostreamsdata.h"
#include "CacheHandlers/soundcachehandler.h"
#include "CacheHandlers/soundcachecontainer.h"
#include "Sound/soundcomposition.h"

void SoundReader::afterProcessing() {
    mCacheHandler->secondLoaderFinished(mSecondId, mSamples);
}

void SoundReader::afterCanceled() {
    mCacheHandler->secondLoaderCanceled(mSecondId);
}

void SoundReader::readFrame() {
    if(!mOpenedAudio->fOpened)
        RuntimeThrow("Cannot read frame from closed AudioStream");
    const auto formatContext = mOpenedAudio->fFormatContext;
    const auto audioStreamIndex = mOpenedAudio->fAudioStreamIndex;
    const auto audioStream = mOpenedAudio->fAudioStream;
    const auto packet = mOpenedAudio->fPacket;
    const auto decodedFrame = mOpenedAudio->fDecodedFrame;
    const auto codecContext = mOpenedAudio->fCodecContext;
    const auto swrContext = mOpenedAudio->fSwrContext;
    //const qreal fps = mOpenedAudio->fFps;

    const int64_t tsms = qFloor(mSecondId * 1000 - 1);
    const int64_t tm = av_rescale(tsms, audioStream->time_base.den,
                                  audioStream->time_base.num)/1000;
    if(tm <= 0)
        avformat_seek_file(formatContext, audioStreamIndex,
                           INT64_MIN, 0, 0, 0);
    else {
        const int64_t tsms0 = qFloor((mSecondId - 1) * 1000);
        const int64_t tm0 = av_rescale(tsms0, audioStream->time_base.den,
                                       audioStream->time_base.num)/1000;
        if(avformat_seek_file(formatContext, audioStreamIndex, tm0,
                              tm, tm, AVSEEK_FLAG_FRAME) < 0) {
            qDebug() << "Failed to seek to " << mSecondId;
            avformat_seek_file(formatContext, audioStreamIndex,
                               INT64_MIN, 0, INT64_MAX, 0);
        }
    }

    avcodec_flush_buffers(codecContext);
    int64_t pts = 0;
    bool firstFrame = true;
    int currentSample = 0;
    float * audioData = nullptr;
    SampleRange audioDataRange{mSampleRange.fMin, mSampleRange.fMin - 1};
    int nSamples = 0;
    while(true) {
        if(av_read_frame(formatContext, packet) < 0) {
            break;
        } else {
            if(packet->stream_index == audioStreamIndex) {
                const int sendRet = avcodec_send_packet(codecContext, packet);
                if(sendRet < 0) {
                    fprintf(stderr, "Sending packet to the decoder failed\n");
                    return;
                }
                const int recRet = avcodec_receive_frame(codecContext, decodedFrame);
                if(recRet == AVERROR_EOF) {
                    return;
                } else if(recRet == AVERROR(EAGAIN)) {
                    av_packet_unref(packet);
                    continue;
                } else if(recRet < 0) {
                    fprintf(stderr, "Did not receive frame from the decoder\n");
                    return;
                }
                av_packet_unref(packet);
            } else {
                av_packet_unref(packet);
                continue;
            }
        }

        // calculate PTS:
        pts = av_frame_get_best_effort_timestamp(decodedFrame);
        pts = av_rescale_q(pts, audioStream->time_base, AV_TIME_BASE_Q);
        const int currSecond = qFloor(pts/1000000.);
        if(currSecond >= mSecondId) {
            if(currSecond > mSecondId)
                qDebug() << QString::number(currSecond) +
                            " instead of " + QString::number(mSecondId);

            // resample frames
            float *buffer;
            av_samples_alloc((uint8_t**)&buffer, nullptr, 1,
                             decodedFrame->nb_samples, AV_SAMPLE_FMT_FLT, 0);
            const int nSamplesT = swr_convert(
                        swrContext, (uint8_t**)&buffer,
                        decodedFrame->nb_samples,
                        (const uint8_t**)decodedFrame->data,
                        decodedFrame->nb_samples);
            // append resampled frames to data
            if(firstFrame) {
                pts = av_rescale_q(pts, audioStream->time_base, AV_TIME_BASE_Q);
                currentSample = pts*SOUND_SAMPLERATE/1000000;
            }
            const SampleRange frameSampleRange{currentSample, nSamplesT};
            const SampleRange neededSampleRange = mSampleRange*frameSampleRange;
            const int firstRelSample = neededSampleRange.fMin - frameSampleRange.fMin;
            const int nSamplesInRange = neededSampleRange.span();
            if(nSamplesInRange > 0) {
                const int newNSamples = nSamples + nSamplesInRange;
                const ulong newAudioDataSize = static_cast<ulong>(newNSamples) * sizeof(float);
                void * const audioDataMem = realloc(audioData, newAudioDataSize);
                audioData = static_cast<float*>(audioDataMem);
                const float * const src = buffer + firstRelSample;
                float * const dst = audioData + nSamples;
                memcpy(dst, src, static_cast<ulong>(nSamplesInRange) * sizeof(float));
                nSamples = newNSamples;
                if(firstFrame) audioDataRange = neededSampleRange;
                else audioDataRange += neededSampleRange;
            }

            av_freep(&((uint8_t**)&buffer)[0]);

            currentSample += nSamplesT;
            firstFrame = false;
            if(currentSample >= mSampleRange.fMax) break;
        }
        av_frame_unref(decodedFrame);
    }

    mSamples = SPtrCreate(Samples)(audioData, audioDataRange);

    av_frame_unref(decodedFrame);
    av_packet_unref(packet);
}