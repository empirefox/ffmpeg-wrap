#include "gang_audio_device.h"

#include "webrtc/base/common.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

#include "gang_spdlog_console.h"

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

SampleData::SampleData(uint8_t* _data, uint32_t _nSamples) :
				data(_data),
				nSamples(_nSamples) {
}

SampleData::~SampleData() {
	free(data);
}

GangAudioDevice::GangAudioDevice(GangDecoder* decoder, int stop_ref_count) :
				ref_count_(0),
				stop_ref_count_(stop_ref_count),
				last_process_time_ms_(0),
				audio_callback_(NULL),
				recording_(false),
				rec_is_initialized_(false),
				decoder_(decoder),
				rec_buff_index_(0),
				len_bytes_per_10ms_(0),
				nb_samples_10ms_(0),
				rec_worker_thread_(new rtc::Thread),
				_recSampleRate(0),
				_recChannels(0),
				_recChannel(AudioDeviceModule::kChannelBoth),
				_recBytesPerSample(0),
				_currentMicLevel(0),
				_newMicLevel(0),
				_typingStatus(false),
				_totalDelayMS(kTotalDelayMs),
				_clockDrift(kClockDriftMs),
				_record_index(0) {

	rec_worker_thread_->Start();
	memset(rec_buff_, 0, kMaxBufferSizeBytes);
	SPDLOG_TRACE(console);
}

GangAudioDevice::~GangAudioDevice() {
	SPDLOG_TRACE(console);
	// Ensure that thread stops calling ProcessFrame().
	{
		rtc::CritScope cs(&lock_);

		if (decoder_) {
			decoder_->SetAudioFrameObserver(NULL);
			decoder_ = NULL;
		}
		if (rec_worker_thread_) {
			rec_worker_thread_->Stop();
			rec_worker_thread_->Clear(this);
			delete rec_worker_thread_;
			rec_worker_thread_ = NULL;
		}
	}
}

rtc::scoped_refptr<GangAudioDevice> GangAudioDevice::Create(
		GangDecoder* decoder,
		int stop_ref_count) {
	if (!decoder) {
		return NULL;
	}
	rtc::scoped_refptr<GangAudioDevice> capture_module(
			new GangAudioDevice(decoder, stop_ref_count));
	capture_module->Initialize();
	return capture_module;
}

int64_t GangAudioDevice::TimeUntilNextProcess() {
//	SPDLOG_TRACE(console);
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
//	SPDLOG_TRACE(console);
	last_process_time_ms_ = rtc::Time();
	return 0;
}

int32_t GangAudioDevice::ActiveAudioLayer(AudioLayer* audio_layer) const {
	SPDLOG_TRACE(console);
	return -1;
}

webrtc::AudioDeviceModule::ErrorCode GangAudioDevice::LastError() const {
	SPDLOG_TRACE(console);
	return webrtc::AudioDeviceModule::kAdmErrNone;
}

int32_t GangAudioDevice::RegisterEventObserver(
		webrtc::AudioDeviceObserver* event_callback) {
	// Only used to report warnings and errors. This fake implementation won't
	// generate any so discard this callback.
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::RegisterAudioCallback(
		webrtc::AudioTransport* audio_callback) {
	rtc::CritScope cs(&lockCb_);
	audio_callback_ = audio_callback;
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::Init() {
	SPDLOG_TRACE(console);
	// Initialize is called by the factory method. Safe to ignore this Init call.
	return 0;
}

int32_t GangAudioDevice::Terminate() {
	SPDLOG_TRACE(console);
	// Clean up in the destructor. No action here, just success.
	return 0;
}

bool GangAudioDevice::Initialized() const {
	SPDLOG_TRACE(console);
	return decoder_ != NULL;
}

int16_t GangAudioDevice::PlayoutDevices() {
	SPDLOG_TRACE(console);
	return 0;
}

int16_t GangAudioDevice::RecordingDevices() {
	SPDLOG_TRACE(console);
	return 1;
}

int32_t GangAudioDevice::PlayoutDeviceName(
		uint16_t index,
		char name[webrtc::kAdmMaxDeviceNameSize],
		char guid[webrtc::kAdmMaxGuidSize]) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::RecordingDeviceName(
		uint16_t index,
		char name[kAdmMaxDeviceNameSize],
		char guid[kAdmMaxGuidSize]) {

	SPDLOG_TRACE(console);
	// TODO give device a actual name
	const char* kName = "gang_audio_device";
	const char* kGuid = "gang_audio_unique_id";
	if (index < 1) {
		memset(name, 0, kAdmMaxDeviceNameSize);
		memset(guid, 0, kAdmMaxGuidSize);
		memcpy(name, kName, strlen(kName));
		memcpy(guid, kGuid, strlen(guid));
		return 0;
	}
	return -1;
}

int32_t GangAudioDevice::SetPlayoutDevice(uint16_t index) {
	// No playout device, just playing from file. Return success.
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::SetPlayoutDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(uint16_t index) {
	SPDLOG_TRACE(console);
	if (index == 0) {
		_record_index = index;
		return _record_index;
	}
	return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::PlayoutIsAvailable(bool* available) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::InitPlayout() {
	SPDLOG_TRACE(console);
	return 0;
}

bool GangAudioDevice::PlayoutIsInitialized() const {
	SPDLOG_TRACE(console);
	return true;
}

int32_t GangAudioDevice::RecordingIsAvailable(bool* available) {
	SPDLOG_TRACE(console);
	if (_record_index == 0) {
		*available = true;
		return _record_index;
	}
	*available = false;
	return -1;
}

int32_t GangAudioDevice::InitRecording() {
	SPDLOG_TRACE(console);
	return 0;
}

bool GangAudioDevice::RecordingIsInitialized() const {
	SPDLOG_TRACE(console);
	return true;
}

int32_t GangAudioDevice::StartPlayout() {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StopPlayout() {
	SPDLOG_TRACE(console);
	return 0;
}

bool GangAudioDevice::Playing() const {
	SPDLOG_TRACE(console);
	return true;
}

// TODO StartRecording
int32_t GangAudioDevice::StartRecording() {
	if (!rec_is_initialized_) {
		return -1;
	}SPDLOG_DEBUG(console);
	rtc::CritScope cs(&lock_);
	recording_ = true;
	rec_buff_index_ = 0;
	decoder_->SetAudioFrameObserver(this);
	return 0;
}

// TODO StopRecording
int32_t GangAudioDevice::StopRecording() {
	SPDLOG_DEBUG(console);
	rtc::CritScope cs(&lock_);
	if (decoder_) {
		decoder_->SetAudioFrameObserver(NULL);
	}
	recording_ = false;
	return 0;
}

bool GangAudioDevice::Recording() const {
	SPDLOG_TRACE(console);
	rtc::CritScope cs(&lock_);
	return recording_;
}

int32_t GangAudioDevice::SetAGC(bool enable) {
	SPDLOG_TRACE(console);
	// No Automatic gain control
	return -1;
}

bool GangAudioDevice::AGC() const {
	SPDLOG_TRACE(console);
	return false;
}

int32_t GangAudioDevice::SetWaveOutVolume(
		uint16_t volumeLeft,
		uint16_t volumeRight) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::WaveOutVolume(
		uint16_t* volume_left,
		uint16_t* volume_right) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::InitSpeaker() {
	SPDLOG_TRACE(console);
	return -1;
}

bool GangAudioDevice::SpeakerIsInitialized() const {
	SPDLOG_TRACE(console);
	return false;
}

int32_t GangAudioDevice::InitMicrophone() {
	SPDLOG_TRACE(console);
	// No microphone, just playing from file. Return success.
	return 0;
}

bool GangAudioDevice::MicrophoneIsInitialized() const {
	SPDLOG_TRACE(console);
	return true;
}

int32_t GangAudioDevice::SpeakerVolumeIsAvailable(bool* available) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetSpeakerVolume(uint32_t /*volume*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SpeakerVolume(uint32_t* /*volume*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MaxSpeakerVolume(uint32_t* /*max_volume*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MinSpeakerVolume(uint32_t* /*min_volume*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SpeakerVolumeStepSize(uint16_t* /*step_size*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeIsAvailable(bool* available) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneVolume(uint32_t volume) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolume(uint32_t* volume) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MaxMicrophoneVolume(uint32_t* max_volume) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MinMicrophoneVolume(uint32_t* /*min_volume*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeStepSize(
		uint16_t* /*step_size*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SpeakerMuteIsAvailable(bool* /*available*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetSpeakerMute(bool /*enable*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SpeakerMute(bool* /*enabled*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneMuteIsAvailable(bool* /*available*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneMute(bool /*enable*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneMute(bool* /*enabled*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneBoostIsAvailable(bool* /*available*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneBoost(bool /*enable*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::MicrophoneBoost(bool* /*enabled*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::StereoPlayoutIsAvailable(bool* available) const {
	SPDLOG_TRACE(console);
	// No recording device, just dropping audio. Stereo can be dropped just
	// as easily as mono.
	*available = true;
	return 0;
}

int32_t GangAudioDevice::SetStereoPlayout(bool /*enable*/) {
	SPDLOG_TRACE(console);
	// No recording device, just dropping audio. Stereo can be dropped just
	// as easily as mono.
	return 0;
}

int32_t GangAudioDevice::StereoPlayout(bool* /*enabled*/) const {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StereoRecordingIsAvailable(bool* available) const {
	SPDLOG_TRACE(console);
	// Keep thing simple. No stereo recording.
	*available = true;
	return 0;
}

int32_t GangAudioDevice::SetStereoRecording(bool enable) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StereoRecording(bool* enabled) const {
	SPDLOG_TRACE(console);
	*enabled = true;
	return 0;
}

int32_t GangAudioDevice::SetRecordingChannel(const ChannelType channel) {
	SPDLOG_TRACE(console);
	rtc::CritScope cs(&lock_);

	if (_recChannels == 1) {
		return -1;
	}

	if (channel == AudioDeviceModule::kChannelBoth) {
		// two bytes per channel
		_recBytesPerSample = 4;
	} else {
		// only utilize one out of two possible channels (left or right)
		_recBytesPerSample = 2;
	}
	_recChannel = channel;

	return 0;
}

int32_t GangAudioDevice::RecordingChannel(ChannelType* channel) const {
	SPDLOG_TRACE(console);
	*channel = _recChannel;
	return 0;
}

int32_t GangAudioDevice::SetPlayoutBuffer(
		const BufferType /*type*/,
		uint16_t /*size_ms*/) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::PlayoutBuffer(
		BufferType* /*type*/,
		uint16_t* /*size_ms*/) const {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::PlayoutDelay(uint16_t* delay_ms) const {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::RecordingDelay(uint16_t* /*delay_ms*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::CPULoad(uint16_t* /*load*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::StartRawOutputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StopRawOutputFileRecording() {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StartRawInputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::StopRawInputFileRecording() {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::SetRecordingSampleRate(
		const uint32_t /*samples_per_sec*/) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::RecordingSampleRate(uint32_t* samples_per_sec) const {
	SPDLOG_TRACE(console);
	if (_recSampleRate == 0) {
		return -1;
	}
	*samples_per_sec = _recSampleRate;
	return 0;
}

int32_t GangAudioDevice::SetPlayoutSampleRate(
		const uint32_t /*samples_per_sec*/) {
	SPDLOG_TRACE(console);
	return 0;
}

int32_t GangAudioDevice::PlayoutSampleRate(
		uint32_t* /*samples_per_sec*/) const {
	SPDLOG_TRACE(console);
	ASSERT(false);
	return 0;
}

int32_t GangAudioDevice::ResetAudioDevice() {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::SetLoudspeakerStatus(bool /*enable*/) {
	SPDLOG_TRACE(console);
	return -1;
}

int32_t GangAudioDevice::GetLoudspeakerStatus(bool* /*enabled*/) const {
	SPDLOG_TRACE(console);
	return -1;
}

void GangAudioDevice::Initialize() {
	SPDLOG_DEBUG(console);
	last_process_time_ms_ = rtc::Time();

	decoder_->GetAudioInfo(&_recSampleRate, &_recChannels);
	_recBytesPerSample = 2 * _recChannels; // 16 bits per sample in mono, 32 bits in stereo
	nb_samples_10ms_ = _recSampleRate / 100; // must fix to rate

	SPDLOG_DEBUG(console,
			"For webrtc, rate: {}, channels: {}, bytesPerSample: {}",
			_recSampleRate,
			_recChannels,
			_recBytesPerSample);

	// init rec_rest_buff_ 10ms container
	len_bytes_per_10ms_ = nb_samples_10ms_ * _recBytesPerSample;

	rec_is_initialized_ = true;
}

bool GangAudioDevice::OnAudioFrame(uint8_t* data, uint32_t nSamples) {
	if (!audio_callback_) {
		return false;
	}
	rec_worker_thread_->Post(
			this,
			MSG_REC_DATA,
			new SampleMsgData(new SampleData(data, nSamples)));
	return true;
}

// ----------------------------------------------------------------------------
//  DeliverRecordedData
// ----------------------------------------------------------------------------

int32_t GangAudioDevice::DeliverRecordedData() {
	uint32_t newMicLevel(0);
	int32_t res = audio_callback_->RecordedDataIsAvailable( //
			rec_buff_, // need interleaved data
			nb_samples_10ms_, // 10ms samples
			_recBytesPerSample,
			_recChannels,
			_recSampleRate,
			_totalDelayMS,
			_clockDrift,
			_currentMicLevel,
			_typingStatus,
			newMicLevel);
	if (res != -1) {
		_newMicLevel = newMicLevel;
	}

	return 0;
}

// ----------------------------------------------------------------------------
//  OnRecData
//
//  Store recorded audio buffer in local memory ready for the actual
//  "delivery" using a callback.
//
//  This method can also parse out left or right channel from a stereo
//  input signal, i.e., emulate mono.
//
//  Examples:
//
//  16-bit,48kHz mono,  10ms => nSamples=480 => _recSize=2*480=960 bytes
//  16-bit,48kHz stereo,10ms => nSamples=480 => _recSize=4*480=1920 bytes
// ----------------------------------------------------------------------------
void GangAudioDevice::OnRecData(uint8_t* data, uint32_t nSamples) {

	uint32_t nb_src_bytes = nSamples * _recBytesPerSample;
	uint32_t src_index = 0;

	// Ensure that user has initialized all essential members
	if ((_recSampleRate == 0) || (nb_src_bytes == 0) || (_recChannels == 0)) {
		return;
	}

	if (AudioDeviceModule::kChannelRight == _recChannel) {
		++src_index;
	}

	do {
		rec_buff_[rec_buff_index_] = data[src_index];
		++rec_buff_index_;
		++src_index;
		if (_recChannel != AudioDeviceModule::kChannelBoth) {
			++src_index;
		}

		if (rec_buff_index_ == len_bytes_per_10ms_) {
			rec_buff_index_ = 0;
			DeliverRecordedData();
		}
	} while (src_index < nb_src_bytes);
}

void GangAudioDevice::OnMessage(rtc::Message* msg) {
	switch (msg->message_id) {
	case MSG_REC_DATA:
		rtc::CritScope cs(&lockCb_);
		rtc::scoped_ptr<SampleMsgData> pdata(
				static_cast<SampleMsgData*>(msg->pdata));
		SampleData* data = pdata->data().get();
		OnRecData(data->data, data->nSamples);
		break;
	}
}

int GangAudioDevice::AddRef() {
	SPDLOG_TRACE(console);
	return rtc::AtomicOps::Increment(&ref_count_);
}

// TODO injected
int GangAudioDevice::Release() {
	int count = rtc::AtomicOps::Decrement(&ref_count_);
	if (count == stop_ref_count_) {
		SPDLOG_TRACE(console);
		StopRecording();
	}
	if (!count) {
		delete this;
	}
	return count;
}

}  // namespace gang
