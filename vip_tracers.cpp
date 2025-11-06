#include <stdio.h>
#include "vip_tracers.h"

vip_tracers g_vip_tracers;

CBulletImpactEvent g_BulletImpactEvent;
IVIPApi* g_pVIPCore;
IUtilsApi* g_pUtils;

IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;

std::vector<CHandle<CBeam>> g_vBeams[64];

PLUGIN_EXPOSE(vip_tracers, g_vip_tracers);

extern ISource2GameEntities* g_pSource2GameEntities;

SH_DECL_HOOK7_void(ISource2GameEntities, CheckTransmit, SH_NOATTRIB, 0, 
    CCheckTransmitInfo **, int, CBitVec<16384> &, CBitVec<16384> &,
    const Entity2Networkable_t **, const uint16 *, int);

Color colorConverter(const char* hexColor)
{
	Color color;
	unsigned int r, g, b;
    std::sscanf(hexColor, "#%02x%02x%02x", &r, &g, &b);
	color.SetColor(r, g, b, 255);
	return color;
}

bool vip_tracers::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
	SH_ADD_HOOK(ISource2GameEntities, CheckTransmit, g_pSource2GameEntities, SH_MEMBER(this, &vip_tracers::Hook_CheckTransmit), true);
	g_SMAPI->AddListener( this, this );
	return true;
}

void vip_tracers::Hook_CheckTransmit(
    CCheckTransmitInfo **ppInfoList, int infoCount,
    CBitVec<16384> &unionTransmitEdicts,
    CBitVec<16384> &unionTransmitAlwaysEdicts,
    const Entity2Networkable_t **pNetworkables,
    const uint16 *pEntityIndicies, int nEntities)
{
    for (int i = 0; i < infoCount; i++)
    {
        auto &pInfo = ppInfoList[i];
        int iViewer = *(uint8*)((uint8*)pInfo + 584);
        CCSPlayerController* pViewer = CCSPlayerController::FromSlot(iViewer);
        if (!pViewer || !pViewer->IsConnected()) continue;

        for (int iShooter = 0; iShooter < 64; iShooter++)
        {
            if (iShooter == iViewer) continue;

            const char* mode = g_pVIPCore->VIP_GetClientFeatureString(iShooter, "tracers_mode");
            if (!mode || !mode[0]) continue;

            CCSPlayerController* pShooter = CCSPlayerController::FromSlot(iShooter);
            if (!pShooter) continue;

            bool sameTeam = (pShooter->m_iTeamNum() == pViewer->m_iTeamNum());

            bool hide = false;
            if (!strcmp(mode, "self"))
                hide = true;
            else if (!strcmp(mode, "team") && !sameTeam)
                hide = true;

            if (!hide) continue;

            for (auto &hBeam : g_vBeams[iShooter])
                if (hBeam)
                    pInfo->m_pTransmitEntity->Clear(hBeam->entindex());
        }
    }
}

bool vip_tracers::Unload(char *error, size_t maxlen)
{
    SH_REMOVE_HOOK(ISource2GameEntities, CheckTransmit, g_pSource2GameEntities, SH_MEMBER(this, &vip_tracers::Hook_CheckTransmit), true);
    delete g_pVIPCore;
    delete g_pUtils;
    return true;
}

CGameEntitySystem* GameEntitySystem()
{
    return g_pVIPCore->VIP_GetEntitySystem();
};

void VIP_OnVIPLoaded()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = g_pGameEntitySystem;
	g_pUtils->GetGameEventManager()->AddListener(&g_BulletImpactEvent, "bullet_impact", true);
}

void DrawLaserBetween(int iSlot, Vector startPos, Vector endPos, Color color)
{
	CBeam* beam = (CBeam*)g_pUtils->CreateEntityByName("beam", -1);
	beam->m_clrRender().SetColor(color.r(), color.g(), color.b(), 255);
	beam->m_fWidth() = g_pVIPCore->VIP_GetClientFeatureFloat(iSlot, "tracers_width");
	beam->m_CBodyComponent()->m_pSceneNode()->m_vecAbsOrigin() = startPos;
	beam->m_vecEndPos() = endPos;
	g_pUtils->DispatchSpawn(beam, 0);
	g_vBeams[iSlot].push_back(CHandle<CBeam>(beam));

	float lifetime = g_pVIPCore->VIP_GetClientFeatureFloat(iSlot, "tracers_time_life");
	g_pUtils->CreateTimer(lifetime, [iSlot, hEntity = CHandle<CBeam>(beam)]()
	{
    	CBeam* entity = hEntity.Get();
    	if (!entity) return -1.0f;

    	g_pUtils->RemoveEntity(entity);

    	auto& beams = g_vBeams[iSlot];
    	beams.erase(std::remove(beams.begin(), beams.end(), hEntity), beams.end());

    	return -1.0f;
	});
}

void CBulletImpactEvent::FireGameEvent(IGameEvent* pEvent)
{
	int iSlot = pEvent->GetInt("userid");
	CCSPlayerController* pPlayer = CCSPlayerController::FromSlot(iSlot);
	if (!pPlayer) return;
	if (!pPlayer->m_hPlayerPawn()) return;

	const char* sTracers = g_pVIPCore->VIP_GetClientFeatureString(iSlot, "tracers");
	if (sTracers && sTracers[0] != '\0')
	{
		Vector PlayerPosition = pPlayer->m_hPlayerPawn()->GetAbsOrigin();
		Vector BulletOrigin = Vector(PlayerPosition.x, PlayerPosition.y, PlayerPosition.z + 64);
		Vector bulletDestination = Vector(pEvent->GetFloat("x"), pEvent->GetFloat("y"), pEvent->GetFloat("z"));

		DrawLaserBetween(
			iSlot,
			BulletOrigin,
			bulletDestination,
			!strcmp(sTracers, "random") ? Color(rand() % 255, rand() % 255, rand() % 255) : colorConverter(sTracers)
		);
	}
}

void vip_tracers::AllPluginsLoaded()
{
	int ret;
	g_pVIPCore = (IVIPApi*)g_SMAPI->MetaFactory(VIP_INTERFACE, &ret, NULL);

	if (ret == META_IFACE_FAILED)
	{
		char error[64];
		V_strncpy(error, "Failed to lookup vip core. Aborting", 64);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pUtils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);

	if (ret == META_IFACE_FAILED)
	{
		char error[64];
		V_strncpy(error, "Failed to lookup utils api. Aborting", 64);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}

	if (ret == META_IFACE_FAILED)
	{
		char error[64];
		V_strncpy(error, "Failed to lookup players api. Aborting", 64);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pVIPCore->VIP_OnVIPLoaded(VIP_OnVIPLoaded);
	g_pVIPCore->VIP_RegisterFeature("tracers", VIP_STRING, TOGGLABLE);
	g_pVIPCore->VIP_RegisterFeature("tracers_time_life", VIP_FLOAT, HIDE);
	g_pVIPCore->VIP_RegisterFeature("tracers_width", VIP_FLOAT, HIDE);
	g_pVIPCore->VIP_RegisterFeature("tracers_mode", VIP_STRING, HIDE);
}

const char *vip_tracers::GetLicense()
{
	return "Public";
}

const char *vip_tracers::GetVersion()
{
	return "1.0";
}

const char *vip_tracers::GetDate()
{
	return __DATE__;
}

const char *vip_tracers::GetLogTag()
{
	return "[VIP-TRACERS]";
}

const char *vip_tracers::GetAuthor()
{
	return "E!N with NovaHost";
}

const char *vip_tracers::GetDescription()
{
	return "";
}

const char *vip_tracers::GetName()
{
	return "[VIP] Tracers";
}

const char *vip_tracers::GetURL()
{
	return "https://nova-hosting.ru?ref=ein";
}
