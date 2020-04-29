#include "DomainGroup.h"

#include <stdio.h>
#include <string.h>

#include <string>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <mutex>

#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <sniper/log/log.h>

#include "Config.h"

#define TIMEOUT_VAL 3

using namespace sniper;

bool ResolveName(std::string domainName,  std::vector<std::string>& newips, uint32_t& minTTL)
{
    unsigned char nsbuf[PACKETSZ * 16] = {0};
    char dispbuf[PACKETSZ * 16] = {};
    if (res_init() == -1)
    {
        log_info("res_init failed.");
        return false;
    }
    int l = res_query(domainName.c_str(), ns_c_any,
                            ns_t_a, nsbuf, sizeof(nsbuf));
    if (l < 0)
    {
        log_info("res_query returned a negative value.");
        return false;
    }
    ns_msg msg;
    if (ns_initparse(nsbuf, l, &msg) == -1)
    {
        log_info("ns_initparse failed.");
        return false;
    }
    size_t msgcnt = ns_msg_count(msg, ns_s_an);
    uint8_t ip[4];
    char ipstr[16];
    uint32_t minttl = 0;
    uint32_t curttl = 0;
    for (size_t i = 0; i < msgcnt; ++i)
    {
        ns_rr rr;
        ns_parserr(&msg, ns_s_an, i, &rr);
        ns_sprintrr(&msg, &rr, NULL,
                    NULL, dispbuf, sizeof(dispbuf));
        if (ns_rr_type(rr) == ns_t_a)
        {
            curttl = ns_rr_ttl(rr);
            memcpy(ip, ns_rr_rdata(rr), sizeof(ip));
            memset(ipstr, 0, sizeof(ipstr));
            snprintf(ipstr, sizeof(ipstr), "%u.%u.%u.%u",
                            ip[0], ip[1], ip[2], ip[3]);
            newips.push_back(ipstr);
            if (minttl == 0)
                    minttl = curttl;
            else
            {
                if (curttl < minttl)
                        minttl = curttl;
            }
        }
    }
    minTTL = minttl;

    return !newips.empty();
}

long CurrentTimeMS()
{
    struct timeval start;
    gettimeofday(&start, NULL);
    return ((start.tv_sec) * 1000 + start.tv_usec/1000.0) + 0.5;
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

//Класс проверки скорости SpeedTester
class SpeedTester
{
public:
    SpeedTester(event::loop_ptr l, uint16_t port,
                uint32_t testi, uint32_t pn, DomainGroup& dg) : srvPort(port), testInterval(testi),
                                                    loop(l), peerNumber(pn), domainGroup(dg), timer(NULL)
    {
    }

    virtual ~SpeedTester()
    {
        if(timer)
            delete timer;
    }

    void SetAddresses(const std::vector<std::string>& newips)
    {
        ips.clear();
        for (size_t i = 0; i < newips.size(); ++i)
            ips.push_back({newips[i], 0});
    }

    void StartTest()
    {
        for (size_t i = 0; i < ips.size(); ++i)
            ips[i].ts = accessTime(ips[i].ip);
        sortIPs();
        std::string fastip = GetFastestIP();
        if (!fastip.empty())
        {
            if (fastip.compare(lastFastIP) != 0)
            {
                log_info("IP address changed to {}", fastip);
                lastFastIP = fastip;
                ((sniper::Config*)domainGroup.GetConfig())->ChangePeer(peerNumber, fastip, srvPort);
            }
        }
        //Test
        PrintTimes();//
    }

    void SetTestTimer()
    {
        timer = new event::TimerRepeat(loop, std::chrono::seconds(testInterval), [this]{
            this->StartTest();
        }
        );
    }

    std::string GetFastestIP()
    {
        std::string ip;
        for (size_t i = 0; i < ips.size(); ++i)
        {
            if (ips[i].ts > 0)
            {
                ip = ips[i].ip;
                break;
            }
        }
        return ip;
    }

    //Test
    void PrintTimes()
    {
        for (size_t i = 0; i < ips.size(); ++i)
            log_info("ip = {}, ts = {}", ips[i].ip.c_str(), ips[i].ts);
    }

private:

    struct ipspeed
    {
        std::string ip;
        long ts;
    };

    bool static comp(ipspeed i, ipspeed j) { return (i.ts < j.ts); }//?

    void sortIPs()
    {
        std::sort(ips.begin(), ips.end(), comp);
    }

    long accessTime(std::string domain)
    {
        std::ostringstream os;
        os << "http://" << domain << ":" << srvPort << "/getinfo";
        long tm = -1;
        auto curl = curl_easy_init();
        if (curl)
        {
            std::string response_string;
            curl_easy_setopt(curl, CURLOPT_URL, os.str().c_str());
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            long start = CurrentTimeMS();
            curl_easy_perform(curl);
            long end = CurrentTimeMS();
            curl_easy_cleanup(curl);
            //Анализируем ответ сервера
            if (!response_string.empty())
            {
                if (isValidResponse(response_string))
                    tm = end - start;
            }
        }
        else
            log_info("curl_easy_init() failed.");

        return tm;
    }

    bool isValidResponse(const std::string& resp)
    {
        rapidjson::Document doc;
        if (!doc.Parse(resp.c_str()).HasParseError())
        {
            if (doc.HasMember("result") && doc["result"].IsObject())
            {
                if (
                    doc["result"].HasMember("version") && doc["result"]["version"].IsString() &&
                    doc["result"].HasMember("mh_addr") && doc["result"]["mh_addr"].IsString()
                    )
                    {
                        std::string addr = doc["result"]["mh_addr"].GetString();
                        if (!addr.empty())
                            return true;
                    }
            }
        }
        return false;
    }

    uint16_t srvPort;
    std::vector<ipspeed> ips;
    const static int TIMEOUT = 3;
    uint32_t testInterval;
    event::loop_ptr loop;
    uint32_t peerNumber;
    DomainGroup& domainGroup;
    event::TimerRepeat* timer;
    std::string lastFastIP;
};

//Класс IPResolver
class IPResolver
{
public:
    IPResolver(event::loop_ptr l, std::string dn, DomainGroup& dg) : domainName(dn), ttl(0),
                                                                speedTester(NULL),
                                                                loop(l), domainGroup(dg), timer(NULL)
    {
    }

    virtual ~IPResolver()
    {
        if (timer)
            delete timer;
    }

	bool Resolve()
	{
        uint32_t minttl = 0;
        std::vector<std::string> newips;

        if (ResolveName(domainName, newips, minttl))
        {
            if (isIPListChanged(newips))
            {
                log_info("IP list changed.");
                if (!newips.empty())
                {
                    ips.assign(newips.begin(), newips.end());
                    std::sort(ips.begin(), ips.end());
                    ttl = minttl;
                    if (speedTester)
                        speedTester->StartTest();
                }
                PrintIPS();
            }
            else
                log_info("IP list not changed.");

            return true;
        }
        else
            return false;
	}

    std::string GetDomainName()
    {
        return domainName;
    }

    uint32_t GetTTL()
    {
        return ttl;
    }

	const std::vector<std::string>& GetAddresses()
	{
		return ips;
	}

    //Test
    void PrintIPS()
    {
        for (size_t i = 0; i < ips.size(); ++i)
            log_info("{}", ips[i].c_str());
        log_info("TTL = {}", ttl);
    }

    void SetResolveTimer()
	{
        timer = new event::TimerRepeat(loop, std::chrono::seconds(ttl), [this]{
            if (this->Resolve())
            {
                SpeedTester* st = this->GetSpeedTester();
                if (st)
                    st->SetAddresses(this->GetAddresses());
            }
        }
        );
	}

	void SetSpeedTester(SpeedTester* st)
	{
        speedTester = st;
	}

	SpeedTester* GetSpeedTester()
	{
        return speedTester;
	}

private:

    bool isIPListChanged(std::vector<std::string> newips)
    {
        if (ips.size() != newips.size())
            return true;
        else
        {
            std::sort(newips.begin(), newips.end());
            return (!std::equal(ips.begin(), ips.end(), newips.begin()));
        }
    }

    std::string domainName;
    uint32_t ttl;
    std::vector<std::string> ips;
    SpeedTester* speedTester;
    event::loop_ptr loop;
    DomainGroup& domainGroup;
    event::TimerRepeat* timer;
};

DomainWatcher::DomainWatcher(event::loop_ptr l, std::string dn, uint16_t port,
                    uint32_t sti, uint32_t pn, DomainGroup& dg) : loop(l), peerNumber(pn), domainGroup(dg)
{
    resolver = new IPResolver(l, dn, domainGroup);
    speedtester = new SpeedTester(l, port, sti, pn, domainGroup);
    domainName = dn;
}

DomainWatcher::~DomainWatcher()
{
    if (resolver)
        delete resolver;
    if (speedtester)
        delete speedtester;
}

bool DomainWatcher::Start()
{
    if (resolver->Resolve())
    {
        speedtester->SetAddresses(resolver->GetAddresses());
        resolver->SetSpeedTester(speedtester);
        speedtester->StartTest();
        speedtester->SetTestTimer();
        resolver->SetResolveTimer();
        return true;
    }
    else
        return false;

}

std::string DomainWatcher::GetFastestIP()
{
    return speedtester->GetFastestIP();
}

uint32_t DomainWatcher::GetPeerNumber()
{
    return peerNumber;
}

std::string DomainWatcher::GetDomainName()
{
    return domainName;
}

DomainGroup::~DomainGroup()
{
    for (size_t i = 0; i < watchers.size(); ++i)
        delete watchers[i];
}

void DomainGroup::AddDomain(std::string dn, uint16_t port,
                uint32_t sti, uint32_t pn)
{
    watchers.push_back(new DomainWatcher(loop, dn, port, sti, pn, *this));
}

void DomainGroup::StartWatchers()
{
    for (size_t i = 0; i < watchers.size(); ++i)
    {
        if (!watchers[i]->Start())
            log_info("Can not start watcher for domain {}", watchers[i]->GetDomainName().c_str());
    }
}

void DomainGroup::SetConfig(void* cfg)
{
    config = cfg;
}

