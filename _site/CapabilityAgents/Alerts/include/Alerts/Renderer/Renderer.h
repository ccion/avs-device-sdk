/*
 * Renderer.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_

#include "Alerts/Renderer/RendererInterface.h"
#include "Alerts/Renderer/RendererObserverInterface.h"

#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

/**
 * An implementation of an alert renderer.  This class is thread-safe.
 */
class Renderer
        : public RendererInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface {
public:
    /**
     * Creates a @c Renderer.
     *
     * @param mediaPlayer the @c MediaPlayerInterface that the @c Renderer object will interact with.
     * @return The @c Renderer object.
     */
    static std::shared_ptr<Renderer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    void setObserver(std::shared_ptr<RendererObserverInterface> observer) override;

    void start(
        const std::string& localAudioFilePath,
        const std::vector<std::string>& urls = std::vector<std::string>(),
        int loopCount = 0,
        std::chrono::milliseconds loopPause = std::chrono::milliseconds{0}) override;

    void stop() override;

    void onPlaybackStarted(SourceId sourceId) override;

    void onPlaybackStopped(SourceId sourceId) override;

    void onPlaybackFinished(SourceId sourceId) override;

    void onPlaybackError(SourceId sourceId, const avsCommon::utils::mediaPlayer::ErrorType& type, std::string error)
        override;

private:
    /// A type that identifies which source is currently being operated on.
    using SourceId = avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;

    /**
     * Constructor.
     *
     * @param mediaPlayer The @c MediaPlayerInterface, which will render audio for an alert.
     */
    Renderer(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /*
     * This function will handle setting an observer.
     *
     * @param observer The observer to set.
     */
    void executeSetObserver(std::shared_ptr<RendererObserverInterface> observer);

    /**
     * This function will start rendering audio for the currently active alert.
     *
     * @param localAudioFilePath The file to be rendered.
     * @param urls A container of urls to be rendered per the above description.
     * @param loopCount The number of times the urls should be rendered.
     * @param loopPauseInMilliseconds The number of milliseconds to pause between rendering url sequences.
     */
    void executeStart(
        const std::string& localAudioFilePath,
        const std::vector<std::string>& urls,
        int loopCount,
        std::chrono::milliseconds loopPause);

    /**
     * This function will stop rendering the currently active alert audio.
     */
    void executeStop();

    /**
     * This function will handle the playbackStarted callback.
     *
     * @param sourceId The sourceId of the media that has started playing.
     */
    void executeOnPlaybackStarted(SourceId sourceId);

    /**
     * This function will handle the playbackStopped callback.
     *
     * @param sourceId The sourceId of the media that has stopped playing.
     */
    void executeOnPlaybackStopped(SourceId sourceId);

    /**
     * This function will handle the playbackFinished callback.
     *
     * @param sourceId The sourceId of the media that has finished playing.
     */
    void executeOnPlaybackFinished(SourceId sourceId);

    /**
     * This function will handle the playbackError callback.
     * @param sourceId The sourceId of the media that encountered the error.
     * @param type The error type.
     * @param error The error string generated by the @c MediaPlayer.
     */
    void executeOnPlaybackError(
        SourceId sourceId,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        const std::string& error);

    /**
     * Utility function to handle notifying our observer, if there is one.
     *
     * @param state The state to notify the observer of.
     * @param message The optional message to pass to the observer.
     */
    void notifyObserver(RendererObserverInterface::State state, const std::string& message = "");

    /**
     * Utility function to set our internal sourceId to a non-assigned state, which is best encapsulated in
     * a function since it is a bit non-intuitive per MediaPlayer's interface.
     */
    void resetSourceId();

    /// @}

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{

    /// The @cMediaPlayerInterface which renders the audio files.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// Our observer.
    std::shared_ptr<RendererObserverInterface> m_observer;

    /// The local audio file to be rendered.  This should always be set as a fallback resource in case the
    /// rendering of a url happens to fail (eg. network is down at that time).
    std::string m_localAudioFilePath;

    /// An optional sequence of urls to be rendered.  If the vector is empty, m_localAudioFilePath will be
    /// rendered instead.
    std::vector<std::string> m_urls;

    /// The next url in the url vector to render.
    int m_nextUrlIndexToRender;

    /// The number of times @c m_urls should be rendered.
    int m_loopCount;

    /// The time to pause between the rendering of the @c m_urls sequence.
    std::chrono::milliseconds m_loopPause;

    /// A flag to capture if the renderer has been asked to stop by its owner.
    bool m_isStopping;

    /// The id associated with the media that our MediaPlayer is currently handling.
    SourceId m_currentSourceId;

    /// @}

    /**
     * The @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_