#include <string>
#include <sniper/event/Loop.h>
#include <sniper/event/Timer.h>

#ifndef DOMAIN_GROUP
#define DOMAIN_GROUP

class IPResolver;
class SpeedTester;
class DomainGroup;
class Config;

using namespace sniper;

class DomainWatcher
{
public:
	DomainWatcher(event::loop_ptr l, std::string dn, uint16_t port,
                    uint32_t sti, uint32_t pn, DomainGroup& dg);

    virtual ~DomainWatcher();

    bool Start();

    std::string GetFastestIP();

    uint32_t GetPeerNumber();

    std::string GetDomainName();

private:
	IPResolver* resolver;
	SpeedTester* speedtester;
	event::loop_ptr loop;
	uint32_t peerNumber;
	std::string domainName;
	DomainGroup& domainGroup;
};

class DomainGroup
{
public:
    DomainGroup(event::loop_ptr l) : loop(l)
    {
        config = NULL;
    }
    virtual ~DomainGroup();

    void AddDomain(std::string dn, uint16_t port,
                    uint32_t sti, uint32_t pn);

    void StartWatchers();

    void SetConfig(void* cfg);
    void* GetConfig(){return config;}

private:
    std::vector<DomainWatcher*> watchers;
    event::loop_ptr loop;
    void* config;
};

bool ResolveName(std::string domainName,  std::vector<std::string>& newips, uint32_t& minTTL);

#endif
