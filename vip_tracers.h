#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include "utlvector.h"
#include "ehandle.h"
#include <sh_vector.h>
#include <entity2/entitysystem.h>
#include "utils.hpp"
#include "module.h"
#include "CCSPlayerController.h"
#include "CParticleSystem.h"
#include "CGameRules.h"
#include "iserver.h"
#include "include/vip.h"
#include "include/menus.h"
#include <string>
#include <ctime>

class vip_tracers : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
public:
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();
private:
    void Hook_CheckTransmit(CCheckTransmitInfo **ppInfoList, int infoCount, CBitVec<16384> &unionTransmitEdicts,
                            CBitVec<16384> &unionTransmitAlwaysEdicts,
                            const Entity2Networkable_t **pNetworkables, const uint16 *pEntityIndicies, int nEntities);
};

class CBulletImpactEvent : public IGameEventListener2
{
	void FireGameEvent(IGameEvent* event) override;
};

extern vip_tracers g_vip_tracers;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_