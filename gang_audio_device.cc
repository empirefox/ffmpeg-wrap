#include "gang_audio_device.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

#include "webrtc/base/common.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

using webrtc::CriticalSectionScoped;

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

GangAudioDevice::GangAudioDevice() :
				last_process_time_ms_(0),
				audio_callback_(NULL),
				recording_(false),
				rec_is_initialized_(false),
				decoder_(NULL),
				rec_buff_index_(0),
				len_bytes_per_10ms_(0),
				nb_samples_10ms_(0),
				rec_worker_thread_(new rtc::Thread),
				_critSect(*CriticalSectionWrapper::CreateCriticalSection()),
				_critSectCb(*CriticalSectionWrapper::CreateCriticalSection()),
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
	printf("GangAudioDevice\n");
	rec_worker_thread_->Start();

	memset(rec_buff_, 0, kMaxBufferSizeBytes);
}

GangAudioDevice::~GangAudioDevice() {
	printf("~GangAudioDevice\n");
	// Ensure that thread stops calling ProcessFrame().
	{
		CriticalSectionScoped lock(&_critSect);

		if (decoder_) {
			decoder_->SetAudioFrameObserver(NULL);
			decoder_ = NULL;
		}
		if (rec_worker_thread_) {
			rec_worker_thread_->Clear(this);
			rec_worker_thread_->Stop();
			delete rec_worker_thread_;
			rec_worker_thread_ = NULL;
		}
	}

	delete &_critSect;
	delete &_critSectCb;
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
//	printf("GangAudioDevice::Process()\n");
	last_process_time_ms_ = rtc::Time();
	return 0;
}

int32_t GangAudioDevice::ActiveAudioLayer(AudioLayer* audio_layer) const {
	return -1;
}

webrtc::AudioDeviceModule::ErrorCode GangAudioDevice::LastError() const {
	printf("GangAudioDevice::LastError()\n");
	return webrtc::AudioDeviceModule::kAdmErrNone;
}

int32_t GangAudioDevice::RegisterEventObserver(
		webrtc::AudioDeviceObserver* event_callback) {
	// Only used to report warnings and errors. This fake implementation won't
	// generate any so discard this callback.
	printf("GangAudioDevice::RegisterEventObserver()\n");
	return 0;
}

int32_t GangAudioDevice::RegisterAudioCallback(
		webrtc::AudioTransport* audio_callback) {
	CriticalSectionScoped lock(&_critSectCb);
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
	return decoder_ != NULL;
}

int16_t GangAudioDevice::PlayoutDevices() {
	printf("GangAudioDevice::PlayoutDevices()\n");
	return 0;
}

int16_t GangAudioDevice::RecordingDevices() {
	printf("GangAudioDevice::RecordingDevices()\n");
	return 1;
}

int32_t GangAudioDevice::PlayoutDeviceName(
		uint16_t index,
		char name[webrtc::kAdmMaxDeviceNameSize],
		char guid[webrtc::kAdmMaxGuidSize]) {
	printf("GangAudioDevice::PlayoutDeviceName()\n");
	return 0;
}

int32_t GangAudioDevice::RecordingDeviceName(
		uint16_t index,
		char name[kAdmMaxDeviceNameSize],
		char guid[kAdmMaxGuidSize]) {

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
	printf("GangAudioDevice::SetPlayoutDevice()\n");
	return 0;
}

int32_t GangAudioDevice::SetPlayoutDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	printf("GangAudioDevice::SetPlayoutDevice()\n");
	return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(uint16_t index) {
	if (index == 0) {
		_record_index = index;
		return _record_index;
	}
	return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	return -1;
}

int32_t GangAudioDevice::PlayoutIsAvailable(bool* available) {
	printf("GangAudioDevice::PlayoutIsAvailable()\n");
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

int32_t GangAudioDevice::RecordingIsAvailable(bool* available) {
	if (_record_index == 0) {
		*available = true;
		return _record_index;
	}
	*available = false;
	return -1;
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
	CriticalSectionScoped lock(&_critSect);
	recording_ = true;
	decoder_->SetAudioFrameObserver(this);
	return 0;
}

// TODO StopRecording
int32_t GangAudioDevice::StopRecording() {
	printf("GangAudioDevice::StopRecording()\n");
	CriticalSectionScoped lock(&_critSect);
	recording_ = false;
	if (decoder_) {
		decoder_->SetAudioFrameObserver(NULL);
	}
	rec_buff_index_ = 0;
	return 0;
}

bool GangAudioDevice::Recording() const {
	printf("GangAudioDevice::Recording()\n");
	CriticalSectionScoped lock(&_critSect);
	return recording_;
}

int32_t GangAudioDevice::SetAGC(bool enable) {
	printf("GangAudioDevice::SetAGC()\n");
	// No Automatic gain control
	return -1;
}

bool GangAudioDevice::AGC() const {
	printf("GangAudioDevice::AGC()\n");
	return false;
}

int32_t GangAudioDevice::SetWaveOutVolume(
		uint16_t volumeLeft,
		uint16_t volumeRight) {
	return -1;
}

int32_t GangAudioDevice::WaveOutVolume(
		uint16_t* volume_left,
		uint16_t* volume_right) const {
	printf("GangAudioDevice::WaveOutVolume()\n");
	return -1;
}

int32_t GangAudioDevice::InitSpeaker() {
	printf("GangAudioDevice::InitSpeaker()\n");
	return -1;
}

bool GangAudioDevice::SpeakerIsInitialized() const {
	printf("GangAudioDevice::SpeakerIsInitialized()\n");
	return false;
}

int32_t GangAudioDevice::InitMicrophone() {
	printf("GangAudioDevice::InitMicrophone()\n");
	// No microphone, just playing from file. Return success.
	return 0;
}

bool GangAudioDevice::MicrophoneIsInitialized() const {
	printf("GangAudioDevice::MicrophoneIsInitialized()\n");
	return true;
}

int32_t GangAudioDevice::SpeakerVolumeIsAvailable(bool* available) {
	printf("GangAudioDevice::SpeakerVolumeIsAvailable()\n");
	return -1;
}

int32_t GangAudioDevice::SetSpeakerVolume(uint32_t /*volume*/) {
	printf("GangAudioDevice::SetSpeakerVolume()\n");
	return -1;
}

int32_t GangAudioDevice::SpeakerVolume(uint32_t* /*volume*/) const {
	printf("GangAudioDevice::SpeakerVolume()\n");
	return -1;
}

int32_t GangAudioDevice::MaxSpeakerVolume(uint32_t* /*max_volume*/) const {
	printf("GangAudioDevice::MaxSpeakerVolume()\n");
	return -1;
}

int32_t GangAudioDevice::MinSpeakerVolume(uint32_t* /*min_volume*/) const {
	printf("GangAudioDevice::MinSpeakerVolume()\n");
	return -1;
}

int32_t GangAudioDevice::SpeakerVolumeStepSize(uint16_t* /*step_size*/) const {
	printf("GangAudioDevice::SpeakerVolumeStepSize()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeIsAvailable(bool* available) {
	printf("GangAudioDevice::MicrophoneVolumeIsAvailable()\n");
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneVolume(uint32_t volume) {
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolume(uint32_t* volume) const {
	return -1;
}

int32_t GangAudioDevice::MaxMicrophoneVolume(uint32_t* max_volume) const {
//	printf("GangAudioDevice::MaxMicrophoneVolume()\n");
	return -1;
}

int32_t GangAudioDevice::MinMicrophoneVolume(uint32_t* /*min_volume*/) const {
	printf("GangAudioDevice::MinMicrophoneVolume()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeStepSize(
		uint16_t* /*step_size*/) const {
	printf("GangAudioDevice::MicrophoneVolumeStepSize()\n");
	return -1;
}

int32_t GangAudioDevice::SpeakerMuteIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::SpeakerMuteIsAvailable()\n");
	return -1;
}

int32_t GangAudioDevice::SetSpeakerMute(bool /*enable*/) {
	printf("GangAudioDevice::SetSpeakerMute()\n");
	return -1;
}

int32_t GangAudioDevice::SpeakerMute(bool* /*enabled*/) const {
	printf("GangAudioDevice::SpeakerMute()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneMuteIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::MicrophoneMuteIsAvailable()\n");
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneMute(bool /*enable*/) {
	printf("GangAudioDevice::SetMicrophoneMute()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneMute(bool* /*enabled*/) const {
	printf("GangAudioDevice::MicrophoneMute()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneBoostIsAvailable(bool* /*available*/) {
	printf("GangAudioDevice::MicrophoneBoostIsAvailable()\n");
	return -1;
}

int32_t GangAudioDevice::SetMicrophoneBoost(bool /*enable*/) {
	printf("GangAudioDevice::SetMicrophoneBoost()\n");
	return -1;
}

int32_t GangAudioDevice::MicrophoneBoost(bool* /*enabled*/) const {
	printf("GangAudioDevice::MicrophoneBoost()\n");
	return -1;
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
	return 0;
}

int32_t GangAudioDevice::StereoRecordingIsAvailable(bool* available) const {
	printf("GangAudioDevice::StereoRecordingIsAvailable()\n");
	// Keep thing simple. No stereo recording.
	*available = true;
	return 0;
}

int32_t GangAudioDevice::SetStereoRecording(bool enable) {
	printf("GangAudioDevice::SetStereoRecording()\n");
	return 0;
}

int32_t GangAudioDevice::StereoRecording(bool* enabled) const {
	printf("GangAudioDevice::StereoRecording()\n");
	*enabled = true;
	return 0;
}

int32_t GangAudioDevice::SetRecordingChannel(const ChannelType channel) {
	CriticalSectionScoped lock(&_critSect);

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
	*channel = _recChannel;
	return 0;
}

int32_t GangAudioDevice::SetPlayoutBuffer(
		const BufferType /*type*/,
		uint16_t /*size_ms*/) {
	printf("GangAudioDevice::SetPlayoutBuffer()\n");
	return 0;
}

int32_t GangAudioDevice::PlayoutBuffer(
		BufferType* /*type*/,
		uint16_t* /*size_ms*/) const {
	printf("GangAudioDevice::PlayoutBuffer()\n");
	return 0;
}

int32_t GangAudioDevice::PlayoutDelay(uint16_t* delay_ms) const {
	printf("GangAudioDevice::PlayoutDelay()\n");
	return 0;
}

int32_t GangAudioDevice::RecordingDelay(uint16_t* /*delay_ms*/) const {
	printf("GangAudioDevice::RecordingDelay()\n");
	return -1;
}

int32_t GangAudioDevice::CPULoad(uint16_t* /*load*/) const {
	printf("GangAudioDevice::CPULoad()\n");
	return -1;
}

int32_t GangAudioDevice::StartRawOutputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	printf("GangAudioDevice::StartRawOutputFileRecording()\n");
	return 0;
}

int32_t GangAudioDevice::StopRawOutputFileRecording() {
	printf("GangAudioDevice::StopRawOutputFileRecording()\n");
	return 0;
}

int32_t GangAudioDevice::StartRawInputFileRecording(
		const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
	printf("GangAudioDevice::StartRawInputFileRecording()\n");
	return 0;
}

int32_t GangAudioDevice::StopRawInputFileRecording() {
	printf("GangAudioDevice::StopRawInputFileRecording()\n");
	return 0;
}

int32_t GangAudioDevice::SetRecordingSampleRate(
		const uint32_t /*samples_per_sec*/) {
	printf("GangAudioDevice::SetRecordingSampleRate()\n");
	return 0;
}

int32_t GangAudioDevice::RecordingSampleRate(uint32_t* samples_per_sec) const {
	printf("GangAudioDevice::RecordingSampleRate()\n");
	if (_recSampleRate == 0) {
		return -1;
	}
	*samples_per_sec = _recSampleRate;
	return 0;
}

int32_t GangAudioDevice::SetPlayoutSampleRate(
		const uint32_t /*samples_per_sec*/) {
	printf("GangAudioDevice::SetPlayoutSampleRate()\n");
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
	return -1;
}

int32_t GangAudioDevice::SetLoudspeakerStatus(bool /*enable*/) {
	printf("GangAudioDevice::SetLoudspeakerStatus()\n");
	return -1;
}

int32_t GangAudioDevice::GetLoudspeakerStatus(bool* /*enabled*/) const {
	printf("GangAudioDevice::GetLoudspeakerStatus()\n");
	return -1;
}

bool GangAudioDevice::Initialize(GangDecoder* decoder) {
	printf("GangAudioDevice::Initialize(GangDecoder* decoder) \n");
	if (!decoder) {
		return false;
	}
	last_process_time_ms_ = rtc::Time();
	decoder_ = decoder;

	decoder_->GetAudioInfo(&_recSampleRate, &_recChannels);
	_recBytesPerSample = 2 * _recChannels; // 16 bits per sample in mono, 32 bits in stereo
	nb_samples_10ms_ = _recSampleRate / 100; // must fix to rate

	printf(
			"rate: %d, channels: %d, bytesPerSample: %d\n",
			_recSampleRate,
			_recChannels,
			_recBytesPerSample);

	// init rec_rest_buff_ 10ms container
	len_bytes_per_10ms_ = nb_samples_10ms_ * _recBytesPerSample;

	WebRtcSpl_Init();
	rec_is_initialized_ = true;
	return true;
}

void GangAudioDevice::OnAudioFrame(void* data, uint32_t nSamples) {
	if (!audio_callback_) {
		return;
	}
	rec_worker_thread_->Post(
			this,
			MSG_REC_DATA,
			new SampleData(data, nSamples));
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
void GangAudioDevice::OnRecData(int8_t* data, uint32_t nSamples) {

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
		CriticalSectionScoped lock(&_critSectCb);
		SampleData* msg_data = static_cast<SampleData*>(msg->pdata);
		int8_t* data = reinterpret_cast<int8_t*>(msg_data->data_);
		uint32_t nSamples = msg_data->nSamples_;
		OnRecData(data, nSamples);
		free(data);
		delete msg_data;
		break;
	}
}

}  // namespace gang
