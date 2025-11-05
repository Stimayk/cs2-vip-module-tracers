#include <stdio.h>
#include "vip_tracers.h"

vip_tracers g_vip_tracers;

CBulletImpactEvent g_BulletImpactEvent;
IVIPApi* g_pVIPCore;
IUtilsApi* g_pUtils;

IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;

PLUGIN_EXPOSE(vip_tracers, g_vip_tracers);

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
	g_SMAPI->AddListener( this, this );
	return true;
}

bool vip_tracers::Unload(char *error, size_t maxlen)
{
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

	float lifetime = g_pVIPCore->VIP_GetClientFeatureFloat(iSlot, "tracers_time");
	g_pUtils->CreateTimer(lifetime, [hEntity = CHandle<CBeam>(beam)]()
	{
		CBeam* entity = hEntity.Get();
		if (!entity) return -1.0f;
		g_pUtils->RemoveEntity(entity);
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
