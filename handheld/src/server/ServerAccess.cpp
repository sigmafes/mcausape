#include "ServerAccess.h"
#include "../util/Sha256.h"
#include "../platform/web/WebBridge.h"
#include "../raknet/GetTime.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

ServerAccess::ServerAccess()
:	_whitelistOn(false),
	_authOn(false),
	_pvp(true),
	_grief(true)
{
}

std::string ServerAccess::key(const std::string& name)
{
	std::string k = name;
	for (unsigned int i = 0; i < k.size(); ++i)
		if (k[i] >= 'A' && k[i] <= 'Z') k[i] = (char) (k[i] + 32);
	return k;
}

bool ServerAccess::validName(const std::string& name)
{
	if (name.empty() || name.size() > 16) return false;
	for (unsigned int i = 0; i < name.size(); ++i) {
		const char c = name[i];
		const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
		if (!ok) return false;
	}
	return true;
}

bool ServerAccess::contains(const std::vector<std::string>& v, const std::string& name)
{
	const std::string k = key(name);
	for (unsigned int i = 0; i < v.size(); ++i)
		if (key(v[i]) == k) return true;
	return false;
}

bool ServerAccess::erase(std::vector<std::string>& v, const std::string& name)
{
	const std::string k = key(name);
	for (unsigned int i = 0; i < v.size(); ++i)
		if (key(v[i]) == k) { v.erase(v.begin() + i); return true; }
	return false;
}

void ServerAccess::load(const std::string& folder)
{
	_folder = folder;
	_whitelistOn = _authOn = false;
	_pvp = _grief = true;
	_worldMode = -1;
	_whitelist.clear();
	_ops.clear();
	_accounts.clear();
	if (folder.empty()) return;

	{
		std::ifstream in((folder + "/server.txt").c_str());
		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
			const std::string::size_type eq = line.find('=');
			if (eq == std::string::npos) continue;
			const std::string k = line.substr(0, eq), v = line.substr(eq + 1);
			if (k == "whitelist") _whitelistOn = (v == "1");
			else if (k == "auth") _authOn = (v == "1");
			else if (k == "pvp") _pvp = (v != "0");
			else if (k == "grief") _grief = (v != "0");
			else if (k == "gamemode") _worldMode = (v == "1") ? 1 : (v == "0") ? 0 : -1;
			else if (k == "wl" && validName(v)) _whitelist.push_back(v);
			else if (k == "op" && validName(v)) _ops.push_back(v);
		}
	}
	{
		std::ifstream in((folder + "/auth.txt").c_str());
		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
			const std::string::size_type t1 = line.find('\t');
			if (t1 == std::string::npos) continue;
			const std::string::size_type t2 = line.find('\t', t1 + 1);
			if (t2 == std::string::npos) continue;
			Account a;
			a.name = line.substr(0, t1);
			a.salt = line.substr(t1 + 1, t2 - t1 - 1);
			a.hash = line.substr(t2 + 1);
			if (validName(a.name) && !a.hash.empty()) _accounts.push_back(a);
		}
	}
}

void ServerAccess::save()
{
	if (_folder.empty()) return;
	{
		std::ofstream out((_folder + "/server.txt").c_str(), std::ios::trunc);
		out << "whitelist=" << (_whitelistOn ? 1 : 0) << "\n";
		out << "auth=" << (_authOn ? 1 : 0) << "\n";
		out << "pvp=" << (_pvp ? 1 : 0) << "\n";
		out << "grief=" << (_grief ? 1 : 0) << "\n";
		if (_worldMode >= 0) out << "gamemode=" << _worldMode << "\n";
		for (unsigned int i = 0; i < _whitelist.size(); ++i) out << "wl=" << _whitelist[i] << "\n";
		for (unsigned int i = 0; i < _ops.size(); ++i) out << "op=" << _ops[i] << "\n";
	}
	{
		std::ofstream out((_folder + "/auth.txt").c_str(), std::ios::trunc);
		for (unsigned int i = 0; i < _accounts.size(); ++i)
			out << _accounts[i].name << "\t" << _accounts[i].salt << "\t" << _accounts[i].hash << "\n";
	}
	// Written to the browser's in-memory file system; this is what gets it to
	// storage that survives the tab.
	WebBridge::flushSaves();
}

void ServerAccess::setWhitelist(bool on) { _whitelistOn = on; save(); }
void ServerAccess::setAuth(bool on) { _authOn = on; save(); }
void ServerAccess::setPvp(bool on) { _pvp = on; save(); }
void ServerAccess::setGrief(bool on) { _grief = on; save(); }
void ServerAccess::setWorldMode(int mode) { _worldMode = mode; save(); }

bool ServerAccess::isWhitelisted(const std::string& name) const { return contains(_whitelist, name); }
bool ServerAccess::addWhitelisted(const std::string& name)
{
	if (contains(_whitelist, name)) return false;
	_whitelist.push_back(name);
	save();
	return true;
}
bool ServerAccess::removeWhitelisted(const std::string& name)
{
	if (!erase(_whitelist, name)) return false;
	save();
	return true;
}

bool ServerAccess::isOp(const std::string& name) const { return contains(_ops, name); }
bool ServerAccess::addOp(const std::string& name)
{
	if (contains(_ops, name)) return false;
	_ops.push_back(name);
	save();
	return true;
}
bool ServerAccess::removeOp(const std::string& name)
{
	if (!erase(_ops, name)) return false;
	save();
	return true;
}

bool ServerAccess::hasAccount(const std::string& name) const
{
	const std::string k = key(name);
	for (unsigned int i = 0; i < _accounts.size(); ++i)
		if (key(_accounts[i].name) == k) return true;
	return false;
}

static std::string makeSalt()
{
	static unsigned int counter = 0;
	std::string seed;
	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%d:%u:%d", (unsigned int) RakNet::GetTimeMS(), (int) time(NULL), ++counter, rand());
	return Sha256::hex(buf).substr(0, 16);
}

bool ServerAccess::registerAccount(const std::string& name, const std::string& password)
{
	if (hasAccount(name)) return false;
	Account a;
	a.name = name;
	a.salt = makeSalt();
	a.hash = Sha256::hex(a.salt + password);
	_accounts.push_back(a);
	save();
	return true;
}

bool ServerAccess::checkPassword(const std::string& name, const std::string& password) const
{
	const std::string k = key(name);
	for (unsigned int i = 0; i < _accounts.size(); ++i)
		if (key(_accounts[i].name) == k)
			return Sha256::hex(_accounts[i].salt + password) == _accounts[i].hash;
	return false;
}
