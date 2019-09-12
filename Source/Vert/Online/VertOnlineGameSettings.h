#pragma once

/**
* General session settings for a Shooter game
*/
class FVertOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FVertOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FVertOnlineSessionSettings() {}
};

/**
* General search setting for a Shooter game
*/
class FVertOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FVertOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FVertOnlineSearchSettings() {}
};

/**
* Search settings for an empty dedicated server to host a match
*/
class FVertOnlineSearchSettingsEmptyDedicated : public FVertOnlineSearchSettings
{
public:
	FVertOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FVertOnlineSearchSettingsEmptyDedicated() {}
};
