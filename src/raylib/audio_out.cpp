#include "audio_out.h"

RaylibSound::RaylibSound() {}
RaylibSound::~RaylibSound() {}

bool RaylibSound::Connect(ISoundSource* src) { return true; }
bool RaylibSound::Disconnect(ISoundSource* src) { return true; }
bool RaylibSound::Update(ISoundSource* src) { return true; }
int  RaylibSound::GetSubsampleTime(ISoundSource* src) { return 0; }

void RaylibSound::Init() {}
void RaylibSound::Cleanup() {}
