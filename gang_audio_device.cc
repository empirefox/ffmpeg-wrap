#include "gang_audio_device.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

#include "webrtc/base/common.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

using webrtc::AudioDeviceModule;

namespace gang {

// Audio sample value that is high enough that it doesn't occur naturally when
// frames are being faked. E.g. NetEq will not generate this large sample value
// unless it has received an audio frame containing a sample of this value.
// Even simpler buffers would likely just contain audio sample values of 0.

// Same value as src/modules/audio_device/main/source/audio_device_config.h in
// https://code.google.com/p/webrtc/
static const uint32 kAdmMaxIdleTimeProcess = 1000;

// Constants here are derived by running VoE using a real ADM.
// The constants correspond to 10ms of mono audio at 44kHz.
static const int kTotalDelayMs = 0;
static const int kClockDriftMs = 0;
static const uint32_t kMaxVolume = 14392;

enum {
	MSG_START_PROCESS, MSG_RUN_PROCESS, MSG_STOP_PROCESS,
};

GangAudioDevice::GangAudioDevice() :
				last_process_time_ms_(0),
				audio_callback_(NULL),
				recording_(false),
				rec_is_initialized_(false),
				current_mic_level_(kMaxVolume),
				started_(false),
				decoder_(NULL),
				channels_(0),
				bytesPerSample_(0),
				sample_rate_(0),
				rec_send_buff_(NULL),
				rec_rest_buff_(NULL),
				rec_rest_buff_size_(0),
				len_bytes_10ms_(0),
				rec_worker_thread_(new rtc::Thread) {
	rec_worker_thread_->Start();
	printf("GangAudioDevice\n");
}

GangAudioDevice::~GangAudioDevice() {
	printf("~GangAudioDevice\n");
	// Ensure that thread stops calling ProcessFrame().
	if (decoder_) {
		decoder_->SetAudioFrameObserver(NULL);
	}
	if (rec_worker_thread_) {
		rec_worker_thread_->Clear(this);
		rec_worker_thread_->Stop();
		delete rec_worker_thread_;
		rec_worker_thread_ = NULL;
	}
}

rtc::scoped_refptr<GangAudioDevice> GangAudioDevice::Create() {
	rtc::scoped_refptr<GangAudioDevice> capture_module(
			new rtc::RefCountedObject<GangAudioDevice>());
	return capture_module;
}

int64_t GangAudioDevice::TimeUntilNextProcess() {
	const uint32 current_time = rtc::Time();
	if (current_time < last_process_time_ms_) {
		// TODO: wraparound could be handled more gracefully.
		return 0;
	}
	const uint32 elapsed_time = current_time - last_process_time_ms_;
	if (kAdmMaxIdleTimeProcess < elapsed_time) {
		return 0;
	}
	return kAdmMaxIdleTimeProcess - elapsed_time;
}

int32_t GangAudioDevice::Process() {
	printf("GangAudioDevice::Process()\n");
	last_process_time_ms_ = rtc::Time();
	return 0;
}

int32_t GangAudioDevice::ActiveAudioLayer(AudioLayer* /*audio_layer*/) const {
	printf("GangAudioDevice::ActiveAudioLayer()\n");
	ASSERT(false);
	return 0;
}

webrtc::AudioDeviceModule::ErrorCode GangAudioDevice::LastError() const {
	printf("GangAudioDevice::LastError()\n");
	ASSERT(false);
	return webrtc::AudioDeviceModule::kAdmErrNone;
}

int32_t GangAudioDevice::RegisterEventObserver(
		webrtc::AudioDeviceObserver* /*event_callback*/) {
	// Only used to report warnings and errors. This fake implementation won't
	// generate any so discard this callback.
	printf("GangAudioDevice::RegisterEventObserver()\n");
	return 0;
}

int32_t GangAudioDevice::RegisterAudioCallback(
		webrtc::AudioTransport* audio_callback) {
	rtc::CritScope cs(&crit_callback_);
	audio_callback_ = audio_callback;
	printf("GangAudioDevice::RegisterAudioCallback()\n");
	return 0;
}

int32_t GangAudioDevice::Init() {
	printf("GangAudioDevice::Init()\n");
	// Initialize is called by the factory method. Safe to ignore this Init call.
	return 0;
}

int32_t GangAudioDevice::Terminate() {
	printf("GangAudioDevice::Terminate()\n");
	// Clean up in the destructor. No action here, just success.
	return 0;
}

bool GangAudioDevice::Initialized() const {
	printf("GangAudioDevice::Initialized()\n");
	if (decoder_)
		return true;
	return false;
}

int16_t GangAudioDevice::PlayoutDevices() {
	printf("GangAudioDevice::PlayoutDevices()\n");
	ASSERT(false);
	return 0;
}

int16_t GangAudioDevice::RecordingDevices() {
	printf("GangAudioDevice::RecordingDevices()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::PlayoutDeviceName(
		uint16_t /*index*/,
		char /*name*/[webrtc::kAdmMaxDeviceNameSize],
		char /*guid*/[webrtc::kAdmMaxGuidSize]) {
	printf("GangAudioDevice::PlayoutDeviceName()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::RecordingDeviceName(
		uint16_t /*index*/,
		char /*name*/[webrtc::kAdmMaxDeviceNameSize],
		char /*guid*/[webrtc::kAdmMaxGuidSize]) {
	printf("GangAudioDevice::RecordingDeviceName()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetPlayoutDevice(uint16_t /*index*/) {
	// No playout device, just playing from file. Return success.
	printf("GangAudioDevice::SetPlayoutDevice()\n");
	return 0;
}

int32_t GangAudioDevice::SetPlayoutDevice(
		AudioDeviceModule::WindowsDeviceType /*device*/) {
	printf("GangAudioDevice::SetPlayoutDevice()\n");
	return 0;
}

int32_t GangAudioDevice::SetRecordingDevice(uint16_t /*index*/) {
	// No recording device, just dropping audio. Return success.
	printf("GangAudioDevice::SetRecordingDevice(uint16_t)\n");
	return 0;
}

int32_t GangAudioDevice::SetRecordingDevice(
		AudioDeviceModule::WindowsDeviceType /*device*/) {
	printf("GangAudioDevice::SetRecordingDevice(WindowsDeviceType)\n");
	if (rec_is_initialized_) {
		return -1;
	}
	return 0;
}

int32_t GangAudioDevice::PlayoutIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::PlayoutIsAvailable()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::InitPlayout() {
	printf("GangAudioDevice::InitPlayout()\n");
	return 0;
}

bool GangAudioDevice::PlayoutIsInitialized() const {
	printf("GangAudioDevice::PlayoutIsInitialized()\n");
	return true;
}

int32_t GangAudioDevice::RecordingIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::RecordingIsAvailable()\n");
	return 0;
}

int32_t GangAudioDevice::InitRecording() {
	printf("GangAudioDevice::InitRecording()\n");
	return 0;
}

bool GangAudioDevice::RecordingIsInitialized() const {
	printf("GangAudioDevice::RecordingIsInitialized()\n");
	return true;
}

int32_t GangAudioDevice::StartPlayout() {
	printf("GangAudioDevice::StartPlayout()\n");
	return 0;
}

int32_t GangAudioDevice::StopPlayout() {
	printf("GangAudioDevice::StopPlayout()\n");
	return 0;
}

bool GangAudioDevice::Playing() const {
	printf("GangAudioDevice::Playing()\n");
	return true;
}

// TODO StartRecording
int32_t GangAudioDevice::StartRecording() {
	if (!rec_is_initialized_) {
		return -1;
	}
	printf("StartRecording\n");
	rtc::CritScope cs(&crit_);
	recording_ = true;
	decoder_->GetAudioInfo(&sample_rate_, &channels_, &bytesPerSample_);
	decoder_->SetAudioFrameObserver(this);
	printf(
			"rate: %d, channels: %d, bytesPerSample: %d\n",
			sample_rate_,
			channels_,
			bytesPerSample_);

	// init rec_rest_buff_ 10ms container
	len_bytes_10ms_ = sample_rate_ * channels_ / 100;
	rec_send_buff_ = new uint8_t[len_bytes_10ms_];
	rec_rest_buff_ = new uint8_t[len_bytes_10ms_];
	return 0;
}

// TODO StopRecording
int32_t GangAudioDevice::StopRecording() {
	printf("GangAudioDevice::StopRecording()\n");
	rtc::CritScope cs(&crit_);
	recording_ = false;
	if (decoder_) {
		decoder_->SetAudioFrameObserver(NULL);
	}
	rec_rest_buff_size_ = 0;
	if (rec_send_buff_) {
		delete[] rec_send_buff_;
		rec_send_buff_ = NULL;
	}
	if (rec_rest_buff_) {
		delete[] rec_rest_buff_;
		rec_rest_buff_ = NULL;
	}
	return 0;
}

bool GangAudioDevice::Recording() const {
	printf("GangAudioDevice::Recording()\n");
	rtc::CritScope cs(&crit_);
	return recording_;
}

int32_t GangAudioDevice::SetAGC(bool /*enable*/) {
	printf("GangAudioDevice::SetAGC()\n");
	// No AGC but not needed since audio is pregenerated. Return success.
	return 0;
}

bool GangAudioDevice::AGC() const {
	printf("GangAudioDevice::AGC()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetWaveOutVolume(
		uint16_t /*volume_left*/,
		uint16_t /*volume_right*/) {
	printf("GangAudioDevice::SetWaveOutVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::WaveOutVolume(
		uint16_t* /*volume_left*/,
		uint16_t* /*volume_right*/) const {
	printf("GangAudioDevice::WaveOutVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::InitSpeaker() {
	printf("GangAudioDevice::InitSpeaker()\n");
	// No speaker, just playing from file. Return success.
	return 0;
}

bool GangAudioDevice::SpeakerIsInitialized() const {
	printf("GangAudioDevice::SpeakerIsInitialized()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::InitMicrophone() {
	printf("GangAudioDevice::InitMicrophone()\n");
	// No microphone, just playing from file. Return success.
	return 0;
}

bool GangAudioDevice::MicrophoneIsInitialized() const {
	printf("GangAudioDevice::MicrophoneIsInitialized()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SpeakerVolumeIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::SpeakerVolumeIsAvailable()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetSpeakerVolume(uint32_t /*volume*/) {
	printf("GangAudioDevice::SetSpeakerVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SpeakerVolume(uint32_t* /*volume*/) const {
	printf("GangAudioDevice::SpeakerVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MaxSpeakerVolume(uint32_t* /*max_volume*/) const {
	printf("GangAudioDevice::MaxSpeakerVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MinSpeakerVolume(uint32_t* /*min_volume*/) const {
	printf("GangAudioDevice::MinSpeakerVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SpeakerVolumeStepSize(uint16_t* /*step_size*/) const {
	printf("GangAudioDevice::SpeakerVolumeStepSize()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneVolumeIsAvailable(bool* available) {
	printf("GangAudioDevice::MicrophoneVolumeIsAvailable()\n");
	*available = true;
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetMicrophoneVolume(uint32_t volume) {
	printf("GangAudioDevice::SetMicrophoneVolume()\n");
	rtc::CritScope cs(&crit_);
	current_mic_level_ = volume;
	return 0;
}

int32_t GangAudioDevice::MicrophoneVolume(uint32_t* volume) const {
	printf("GangAudioDevice::MicrophoneVolume()\n");
	rtc::CritScope cs(&crit_);
	*volume = current_mic_level_;
	return 0;
}

int32_t GangAudioDevice::MaxMicrophoneVolume(uint32_t* max_volume) const {
	printf("GangAudioDevice::MaxMicrophoneVolume()\n");
	*max_volume = kMaxVolume;
	return 0;
}

int32_t GangAudioDevice::MinMicrophoneVolume(uint32_t* /*min_volume*/) const {
	printf("GangAudioDevice::MinMicrophoneVolume()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneVolumeStepSize(
		uint16_t* /*step_size*/) const {
	printf("GangAudioDevice::MicrophoneVolumeStepSize()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SpeakerMuteIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::SpeakerMuteIsAvailable()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetSpeakerMute(bool /*enable*/) {
	printf("GangAudioDevice::SetSpeakerMute()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SpeakerMute(bool* /*enabled*/) const {
	printf("GangAudioDevice::SpeakerMute()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneMuteIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::MicrophoneMuteIsAvailable()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetMicrophoneMute(bool /*enable*/) {
	printf("GangAudioDevice::SetMicrophoneMute()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneMute(bool* /*enabled*/) const {
	printf("GangAudioDevice::MicrophoneMute()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneBoostIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::MicrophoneBoostIsAvailable()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetMicrophoneBoost(bool /*enable*/) {
	printf("GangAudioDevice::SetMicrophoneBoost()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::MicrophoneBoost(bool* /*enabled*/) const {
	printf("GangAudioDevice::MicrophoneBoost()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StereoPlayoutIsAvailable(bool* available) const {
	printf("GangAudioDevice::StereoPlayoutIsAvailable()\n");
	// No recording device, just dropping audio. Stereo can be dropped just
	// as easily as mono.
	*available = true;
	return 0;
}

int32_t GangAudioDevice::SetStereoPlayout(bool /*enable*/) {
	printf("GangAudioDevice::SetStereoPlayout()\n");
	// No recording device, just dropping audio. Stereo can be dropped just
	// as easily as mono.
	return 0;
}

int32_t GangAudioDevice::StereoPlayout(bool* /*enabled*/) const {
	printf("GangAudioDevice::StereoPlayout()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StereoRecordingIsAvailable(bool* available) const {
	printf("GangAudioDevice::StereoRecordingIsAvailable()\n");
	// Keep thing simple. No stereo recording.
	*available = false;
	return 0;
}

int32_t GangAudioDevice::SetStereoRecording(bool enable) {
	printf("GangAudioDevice::SetStereoRecording()\n");
	if (!enable) {
		printf("GangAudioDevice::SetStereoRecording() to false\n");
		return 0;
	}
	return -1;
}

int32_t GangAudioDevice::StereoRecording(bool* /*enabled*/) const {
	printf("GangAudioDevice::StereoRecording()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetRecordingChannel(const ChannelType channel) {
	printf("GangAudioDevice::SetRecordingChannel()\n");
	if (channel != AudioDeviceModule::kChannelBoth) {
		printf("GangAudioDevice::SetRecordingChannel()!=2\n");
		// There is no right or left in mono. I.e. kChannelBoth should be used for
		// mono.
		ASSERT(false);
		return -1;
	}
	return 0;
}

int32_t GangAudioDevice::RecordingChannel(ChannelType* channel) const {
	printf("GangAudioDevice::RecordingChannel()\n");
	// Stereo recording not supported. However, WebRTC ADM returns kChannelBoth
	// in that case. Do the same here.
	*channel = AudioDeviceModule::ChannelType::kChannelBoth;
	return 0;
}

int32_t GangAudioDevice::SetPlayoutBuffer(
		const BufferType /*type*/,
		uint16_t /*size_ms*/) {
	printf("GangAudioDevice::SetPlayoutBuffer()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::PlayoutBuffer(
		BufferType* /*type*/,
		uint16_t* /*size_ms*/) const {
	printf("GangAudioDevice::PlayoutBuffer()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::PlayoutDelay(uint16_t* delay_ms) const {
	printf("GangAudioDevice::PlayoutDelay()\n");
	// No delay since audio frames are dropped.
	*delay_ms = 0;
	return 0;
}

int32_t GangAudioDevice::RecordingDelay(uint16_t* /*delay_ms*/) const {
	printf("GangAudioDevice::RecordingDelay()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::CPULoad(uint16_t* /*load*/) const {
	printf("GangAudioDevice::CPULoad()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StartRawOutputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	printf("GangAudioDevice::StartRawOutputFileRecording()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StopRawOutputFileRecording() {
	printf("GangAudioDevice::StopRawOutputFileRecording()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StartRawInputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	printf("GangAudioDevice::StartRawInputFileRecording()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::StopRawInputFileRecording() {
	printf("GangAudioDevice::StopRawInputFileRecording()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetRecordingSampleRate(
		const uint32_t /*samples_per_sec*/) {
	printf("GangAudioDevice::SetRecordingSampleRate()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::RecordingSampleRate(
		uint32_t* /*samples_per_sec*/) const {
	printf("GangAudioDevice::RecordingSampleRate()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetPlayoutSampleRate(
		const uint32_t /*samples_per_sec*/) {
	printf("GangAudioDevice::SetPlayoutSampleRate()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::PlayoutSampleRate(
		uint32_t* /*samples_per_sec*/) const {
	printf("GangAudioDevice::PlayoutSampleRate()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::ResetAudioDevice() {
	printf("GangAudioDevice::ResetAudioDevice()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::SetLoudspeakerStatus(bool /*enable*/) {
	printf("GangAudioDevice::SetLoudspeakerStatus()\n");
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::GetLoudspeakerStatus(bool* /*enabled*/) const {
	printf("GangAudioDevice::GetLoudspeakerStatus()\n");
	ASSERT(false);
	return 0;
}

bool GangAudioDevice::Initialize(GangDecoder* decoder) {
	printf("GangAudioDevice::Initialize(GangDecoder* decoder) \n");
	if (!decoder) {
		return false;
	}
	last_process_time_ms_ = rtc::Time();
	decoder_ = decoder;
	WebRtcSpl_Init();
	rec_is_initialized_ = true;
	return true;
}

void GangAudioDevice::OnAudioFrame(void* data, uint32_t nSamples) {
	printf("GangAudioDevice::OnAudioFrame(), nSamples:%d\n", nSamples);
	if (!audio_callback_) {
		return;
	}
	rec_worker_thread_->Clear(this, MSG_REC_DATA);
	rec_worker_thread_->Post(
			this,
			MSG_REC_DATA,
			new SampleData(data, nSamples));
}

void GangAudioDevice::OnMessage(rtc::Message* msg) {
	switch (msg->message_id) {
	case MSG_REC_DATA:
		bool key_pressed = false;
		uint32_t current_mic_level = 0;
		MicrophoneVolume(&current_mic_level);
		SampleData* msg_data = static_cast<SampleData*>(msg->pdata);
		printf(
				"GangAudioDevice::OnMessage(), nSamples:%d\n",
				msg_data->nSamples_);
		uint8_t* data = reinterpret_cast<uint8_t*>(msg_data->data_);

		uint32_t total_index = 0;
		int to_index = rec_rest_buff_size_;
		do {
			rec_send_buff_[to_index] = data[total_index];
			++total_index;
			++to_index;
			if (to_index == len_bytes_10ms_) {
				to_index = 0;

				int32_t ret = audio_callback_->RecordedDataIsAvailable(
						rec_send_buff_,
						len_bytes_10ms_,
						bytesPerSample_,
						channels_,
						sample_rate_,
						kTotalDelayMs,
						kClockDriftMs,
						current_mic_level,
						key_pressed,
						current_mic_level);
				if (ret != 0)
					printf("GangAudioDevice::OnMessage(), return:%d\n", ret);

				SetMicrophoneVolume(current_mic_level);
			}
		} while (total_index < msg_data->nSamples_);

		rec_rest_buff_size_ = to_index;

		free(data);
		delete msg_data;

		break;
	}
}

}  // namespace gang
