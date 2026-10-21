#ifndef NET_MINECRAFT__DeployNumber_H__
#define NET_MINECRAFT__DeployNumber_H__

#include <string>

/** Which deploy this is, shown on the main menu so a tab can tell whether it is
    running the build that was just published. The deploy workflows pass the
    number of commits on main (MC_DEPLOY_NUMBER): it goes up with every change,
    and both sites get the same one for the same commit. A local build has none. */
#ifndef MC_DEPLOY_NUMBER
#define MC_DEPLOY_NUMBER 0
#endif

inline std::string deployLabel()
{
	if (MC_DEPLOY_NUMBER <= 0) return "Deploy (local)";
	char buf[32];
	snprintf(buf, sizeof(buf), "Deploy %d", (int) MC_DEPLOY_NUMBER);
	return buf;
}

#endif /*NET_MINECRAFT__DeployNumber_H__*/
