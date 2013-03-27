diff --git a/lib/libpacman/db.c b/lib/libpacman/db.c
index 46a6c1f..8d4649e 100644
--- a/lib/libpacman/db.c
+++ b/lib/libpacman/db.c
@@ -23,12 +23,13 @@
  *  USA.
  */
 
+#include "config.h"
+
 #if defined(__APPLE__) || defined(__OpenBSD__)
 #include <sys/syslimits.h>
 #include <sys/stat.h>
 #endif
 
-#include "config.h"
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
@@ -40,12 +41,14 @@
 #ifdef CYGWIN
 #include <limits.h> /* PATH_MAX */
 #endif
+
 /* pacman-g2 */
+#include "db.h"
+
 #include "log.h"
 #include "util.h"
 #include "error.h"
 #include "server.h"
-#include "db.h"
 #include "handle.h"
 #include "cache.h"
 #include "pacman.h"
@@ -90,22 +93,66 @@ int _pacman_db_cmp(const void *db1, const void *db2)
 	return(strcmp(((pmdb_t *)db1)->treename, ((pmdb_t *)db2)->treename));
 }
 
+#include <regex.h>
+typedef struct pmregex pmregex_t;
+struct pmregex {
+	regex_t reg;
+	char *regex;
+	int cflags;
+};
+
+static
+int _pacman_reg_comp (pmregex_t *preg, const char *regex, int cflags) {
+	int ret;
+	if ((ret = regcomp (&preg->reg, regex, cflags)) != 0) {
+		return ret;
+	}
+	if ((preg->regex = strdup(regex)) == NULL) {
+		regfree(&preg->reg);
+		return REG_ESIZE;
+	}
+	preg->cflags = cflags;
+	return 0;
+}
+
 static
-int _pacman_reg_match_or_strstr(const char *string, const char *pattern) {
-	if (_pacman_reg_match(string, pattern) > 0 ||
-			strstr(string, pattern) != NULL) {
-		return 0;
+int __pacman_reg_match (pmregex_t *preg, const char *string, int eflags) {
+	if (regexec(&preg->reg, string, 0, NULL, eflags) == 0) {
+	 	return 0;
+	}
+	/* if (substring match enabled) */ {
+		char *(*_strstr)(const char *, const char *) =
+				(preg->cflags & REG_ICASE) ? strcasestr : strstr;
+		if (_strstr(string, preg->regex) != NULL) {
+			return 0;
+		}
 	}
 	return 1;
 }
 
+static
+void _pacman_reg_free (pmregex_t *preg) {
+	regfree(&preg->reg);
+	free(preg->regex);
+}
+
+static
+int _pacman_str_match (const char *string, pmregex_t *preg) {
+	return __pacman_reg_match (preg, string, 0);
+}
+
+static
+int _pacman_strlist_match (pmlist_t *list, pmregex_t *preg) {
+	return _pacman_list_detect (list, (_pacman_fn_detect)_pacman_str_match, preg);
+}
+
 pmlist_t *_pacman_db_search(pmdb_t *db, pmlist_t *needles)
 {
 	pmlist_t *i, *j, *ret = NULL;
 
 	for(i = needles; i; i = i->next) {
-		/* FIXME: precompile regex once per loop, and handle bad regexp more gracefully */
 		const char *targ;
+		pmregex_t reg;
 
 		if(i->data == NULL) {
 			continue;
@@ -114,16 +161,21 @@ pmlist_t *_pacman_db_search(pmdb_t *db, pmlist_t *needles)
 		targ = i->data;
 		_pacman_log(PM_LOG_DEBUG, "searching for target '%s'\n", targ);
 
+		if (_pacman_reg_comp (&reg, targ, REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
+			RET_ERR(PM_ERR_INVALID_REGEX, -1);
+		}
+
 		for(j = _pacman_db_get_pkgcache(db); j; j = j->next) {
 			pmpkg_t *pkg = j->data;
 
-			if (_pacman_reg_match_or_strstr(pkg->name, targ) == 0 ||
-					_pacman_reg_match_or_strstr(_pacman_pkg_getinfo(pkg, PM_PKG_DESC), targ) == 0 ||
-					_pacman_list_detect(_pacman_pkg_getinfo(pkg, PM_PKG_PROVIDES), (_pacman_fn_detect)_pacman_reg_match_or_strstr, (void *)targ)) {
+			if (_pacman_str_match(pkg->name, &reg) == 0 ||
+					_pacman_str_match(_pacman_pkg_getinfo(pkg, PM_PKG_DESC), &reg) == 0 ||
+					_pacman_strlist_match(_pacman_pkg_getinfo(pkg, PM_PKG_PROVIDES), &reg)) {
 				_pacman_log(PM_LOG_DEBUG, "    search target '%s' matched '%s'", targ, pkg->name);
 				ret = _pacman_list_add(ret, pkg);
 			}
 		}
+		_pacman_reg_free (&reg);
 	}
 
 	return(ret);
