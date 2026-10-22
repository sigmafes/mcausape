#ifndef NET_MINECRAFT_SERVER__ServerAccess_H__
#define NET_MINECRAFT_SERVER__ServerAccess_H__

#include <string>
#include <vector>

/** Who may be in a hosted world and who may give it orders: the whitelist, the
    operators and the /register accounts. Per world, kept in the world's folder
    (server.txt and auth.txt) so they travel with it -- an exported world takes
    its operators along.

    Names are compared without regard to case, the way a player would expect
    "Steve" and "steve" to be one person. Passwords are never kept: an account
    is a salted SHA-256 of the password. */
class ServerAccess
{
public:
	ServerAccess();

	/// Reads the files in `folder` (nothing there is fine: everything is off).
	void load(const std::string& folder);

	bool whitelistOn() const { return _whitelistOn; }
	void setWhitelist(bool on);
	bool isWhitelisted(const std::string& name) const;
	bool addWhitelisted(const std::string& name);		///< false if already there
	bool removeWhitelisted(const std::string& name);	///< false if not there
	const std::vector<std::string>& whitelisted() const { return _whitelist; }

	bool isOp(const std::string& name) const;
	bool addOp(const std::string& name);
	bool removeOp(const std::string& name);

	/// /pvp: players may hurt each other. /grief: visitors may change the world
	/// (blocks, containers, animals and enemies). Both are on until turned off.
	/// /motd: a line under the world in the online list (& codes allowed).
	const std::string& motd() const { return _motd; }
	void setMotd(const std::string& motd);
	bool pvpOn() const { return _pvp; }
	void setPvp(bool on);
	bool griefOn() const { return _grief; }
	void setGrief(bool on);

	/// /gamemode world: what visitors start in, 0 survival or 1 creative; -1
	/// (never set) means whatever the host is playing in, as before.
	int worldMode() const { return _worldMode; }
	void setWorldMode(int mode);

	bool authOn() const { return _authOn; }
	void setAuth(bool on);
	bool hasAccount(const std::string& name) const;
	/// False if the name already has one.
	bool registerAccount(const std::string& name, const std::string& password);
	bool checkPassword(const std::string& name, const std::string& password) const;

	/// Lower-case, for comparing names.
	static std::string key(const std::string& name);
	/// A name a player may have: 1-16 letters, digits, '_' or '-'.
	static bool validName(const std::string& name);

private:
	std::string _motd;
	struct Account { std::string name, salt, hash; };

	void save();
	static bool contains(const std::vector<std::string>& v, const std::string& name);
	static bool erase(std::vector<std::string>& v, const std::string& name);

	std::string _folder;
	bool _whitelistOn;
	bool _authOn;
	bool _pvp;
	int _worldMode;
	bool _grief;
	std::vector<std::string> _whitelist;
	std::vector<std::string> _ops;
	std::vector<Account> _accounts;
};

#endif /*NET_MINECRAFT_SERVER__ServerAccess_H__*/
