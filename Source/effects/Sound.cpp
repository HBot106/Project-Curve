#include "Sound.h"

Sound::Sound() {
	sfxEngine = irrklang::createIrrKlangDevice();

	sfxSources.impactSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/marble_impact.ogg", irrklang::ESM_NO_STREAMING, true);
	sfxSources.resetSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/marble_reset.ogg", irrklang::ESM_NO_STREAMING, true);
	sfxSources.jumpSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/marble_jump.ogg", irrklang::ESM_NO_STREAMING, true);
	sfxSources.winSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/marble_win.ogg", irrklang::ESM_NO_STREAMING, true);
	sfxSources.superBounceSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/marble_super_bounce.ogg", irrklang::ESM_NO_STREAMING, true);
	sfxSources.fanSoundSource = sfxEngine->addSoundSourceFromFile("./resources/sounds/fan.ogg", irrklang::ESM_AUTO_DETECT, true);

	sfxSounds.impactSound = 0;
	sfxSounds.resetSound = 0;
	sfxSounds.jumpSound = 0;
	sfxSounds.winSound = 0;
	sfxSounds.superBounceSound = 0;
	sfxSounds.fanSound = 0;

	// loops over the number of tracks, initializing them and adding them to a vector
	// double completion = 0.0;
	for (int i = 1; i < NUMBER_OF_MUSIC_TRACKS + 1; i++) {
		string file = "./resources/sounds/music/" + to_string(i) + ".ogg";

		irrklang::ISoundSource* musicSource = sfxEngine->addSoundSourceFromFile(file.c_str(), irrklang::ESM_AUTO_DETECT);
		musicSource->setDefaultVolume(0.10f);
		musicSources.push_back(musicSource);

		irrklang::ISound* musicSound = sfxEngine->play2D(musicSources[i - 1], false, true);
        musicSound->setVolume(0.10f);
		musicSounds.push_back(musicSound);

		// completion = ((float)i * 100 / (float)NUMBER_OF_MUSIC_TRACKS);
		// cout << setprecision(3) << "Loading Music: " << completion << "% complete." << endl;
	}

	srand((int)static_cast<int>(time(0)));
	int i = (rand() % NUMBER_OF_MUSIC_TRACKS);
	currentTrack = i;
}

Sound::~Sound() {
	sfxEngine->drop();
}

void Sound::impact(float impactVal) {
	if (!sfxSounds.impactSound) {
		float soundScale = min(1.0f, max(0.0f, ((impactVal * 2.0f) / 100.0f)));

		sfxSounds.impactSound = sfxEngine->play2D(sfxSources.impactSoundSource, false, true);
		sfxSounds.impactSound->setVolume(soundScale);
		sfxSounds.impactSound->setIsPaused(false);
	}
	if (sfxSounds.impactSound) {
		sfxSounds.impactSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.impactSound = 0;
	}
}

void Sound::reset() {
	if (!sfxSounds.resetSound) {
		sfxSounds.resetSound = sfxEngine->play2D(sfxSources.resetSoundSource, false, true);
		sfxSounds.resetSound->setVolume(0.4f);
		sfxSounds.resetSound->setIsPaused(false);
	}
	if (sfxSounds.resetSound) {
		sfxSounds.resetSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.resetSound = 0;
	}
}

void Sound::jump() {
	if (!sfxSounds.resetSound) {
		sfxSounds.jumpSound = sfxEngine->play2D(sfxSources.jumpSoundSource, false, true);
		sfxSounds.jumpSound->setVolume(0.4f);
		sfxSounds.jumpSound->setIsPaused(false);
	}
	if (sfxSounds.jumpSound) {
		sfxSounds.jumpSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.jumpSound = 0;
	}
}

void Sound::superBounce() {
	if (!sfxSounds.superBounceSound) {
		sfxSounds.superBounceSound = sfxEngine->play2D(sfxSources.superBounceSoundSource, false, true);
		sfxSounds.superBounceSound->setVolume(0.4f);
		sfxSounds.superBounceSound->setIsPaused(false);
	}
	if (sfxSounds.superBounceSound) {
		sfxSounds.superBounceSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.superBounceSound = 0;
	}
}

void Sound::win() {
	if (!sfxSounds.winSound) {
		sfxSounds.winSound = sfxEngine->play2D(sfxSources.winSoundSource, false);
	}
	if (sfxSounds.winSound) {
		sfxSounds.winSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.winSound = 0;
	}
}

void Sound::fan(float x, float y, float z) {

	irrklang::vec3df position(x, y, z);

	if (!sfxSounds.fanSound) {
		sfxSounds.fanSound = sfxEngine->play3D(sfxSources.fanSoundSource, position, true, true);
	}

	if (sfxSounds.fanSound) {
		sfxSounds.fanSound->setMinDistance(10.0f);
        sfxSounds.fanSound->setVolume(1.0f);
		sfxSounds.fanSound->setIsPaused(false);	 // unpause the sound
	}

	// if (!sfxSounds.fanSound) {
	// 	sfxSounds.fanSound = sfxEngine->play2D(sfxSources.fanSoundSource, true, true);
	// 	sfxSounds.fanSound->setVolume(0.5f);
	// 	sfxSounds.fanSound->setIsPaused(false);
	// }

	if (sfxSounds.fanSound) {
		sfxSounds.fanSound->drop();  // don't forget to release the pointer once it is no longer needed by you
		sfxSounds.fanSound = 0;
	}
}

void Sound::playPauseMusic() {
	musicSounds[currentTrack]->setIsPaused(!musicSounds[currentTrack]->getIsPaused());
}

void Sound::nextTrackMusic() {
	musicSounds[currentTrack]->setIsPaused(true);
	currentTrack = (currentTrack + 1) % NUMBER_OF_MUSIC_TRACKS;
	musicSounds[currentTrack]->setIsPaused(false);
}

void Sound::updateMusic() {
	if (musicSounds[currentTrack]->isFinished()) {
		musicSounds[currentTrack] = sfxEngine->play2D(musicSources[currentTrack], false, true);
        musicSounds[currentTrack]->setVolume(0.10f);
		nextTrackMusic();
	}
}

void Sound::update3dAudio(irrklang::vec3df pos, irrklang::vec3df look, irrklang::vec3df up) {
	if (musicSounds[currentTrack]->isFinished()) {
		musicSounds[currentTrack] = sfxEngine->play2D(musicSources[currentTrack], false, true);
        musicSounds[currentTrack]->setVolume(0.10f);
		nextTrackMusic();
	}

	irrklang::vec3df position = pos;		 // position of the listener
	irrklang::vec3df lookDirection = look;   // the direction the listener looks into
	irrklang::vec3df velPerSecond(0, 0, 0);  // only relevant for doppler effects
	irrklang::vec3df upVector = up;			 // where 'up' is in your 3D scene

	sfxEngine->setListenerPosition(position, lookDirection, velPerSecond, upVector);
}