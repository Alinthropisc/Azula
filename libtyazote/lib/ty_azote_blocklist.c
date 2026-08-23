#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ty_azote_blocklist.h"
#include "ty_azote_tracing.h"

struct ty_azote_Blocklist {
    ty_azote_Constraint *constraint;
};

static bool looks_like_ipv6(const char *ip)
{
    char buf[128];
    strncpy(buf, ip, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    char *slash = strchr(buf, '/');

    if (slash)
    {
        *slash = '\0';
    }
    struct in6_addr a6;
    return inet_pton(AF_INET6, buf, &a6) == 1;
}

static ty_azote_BlStatus apply_entry(ty_azote_Constraint *c, const char *entry,ty_azote_AddrState state)
{
    if (looks_like_ipv6(entry))
    {
        TY_AZOTE_DEBUG("blocklist: ignoring IPv6 entry (unsupported): %s", entry);
        return TY_AZOTE_BL_OK; /* matches original zmap behavior: skip, not fatal */
    }
    char buf[256];
    strncpy(buf, entry, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    int prefix_len = 32;
    char *slash = strchr(buf, '/');

    if (slash)
    {
        *slash = '\0';
        char *end;
        errno = 0;
        long v = strtol(slash + 1, &end, 10);

        if (end == slash + 1 || errno != 0 || v < 0 || v > 32)
        {
            TY_AZOTE_ERROR("blocklist: '%s' is not a valid IPv4 prefix length", slash + 1);
            return TY_AZOTE_BL_ERR_PARSE_ENTRY;
        }
        prefix_len = (int)v;
    }
    struct in_addr addr;

    if (inet_aton(buf, &addr) != 0)
    {
        ty_azote_constraint_set(c, ntohl(addr.s_addr), prefix_len, state);
        return TY_AZOTE_BL_OK;
    }

    /* Not a literal IP — try DNS resolution (IPv4 only). */
    struct addrinfo hint = {.ai_family = AF_INET}, *res = nullptr;

    if (getaddrinfo(buf, nullptr, &hint, &res) != 0)
    {
        TY_AZOTE_ERROR("blocklist: '%s' is not a valid IP address or hostname", buf);
        return TY_AZOTE_BL_ERR_PARSE_ENTRY;
    }
    int resolved_any = 0;

    for (struct addrinfo *ai = res; ai; ai = ai->ai_next)
    {
        if (ai->ai_family != AF_INET)
        {
            continue;
        }
        struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
        ty_azote_constraint_set(c, ntohl(sa->sin_addr.s_addr), prefix_len, state);
        resolved_any = 1;
        TY_AZOTE_DEBUG("blocklist: '%s' resolved via DNS to %s", buf, inet_ntoa(sa->sin_addr));
    }
    freeaddrinfo(res);
    return resolved_any ? TY_AZOTE_BL_OK : TY_AZOTE_BL_ERR_PARSE_ENTRY;
}

static ty_azote_BlStatus apply_file(ty_azote_Constraint *c, const char *path,ty_azote_AddrState state, bool ignore_invalid)
{
    FILE *fp = fopen(path, "r");

    if (!fp)
    {
        TY_AZOTE_ERROR("blocklist: unable to open '%s': %s", path, strerror(errno));
        return TY_AZOTE_BL_ERR_FILE_OPEN;
    }
    char line[1024];

    while (fgets(line, sizeof line, fp))
    {
        char *comment = strchr(line, '#');

        if (comment)
        {
            *comment = '\0';
        }
        char token[256];

        if (sscanf(line, "%255s", token) != 1)
        {
            continue;
        }
        ty_azote_BlStatus st = apply_entry(c, token, state);

        if (st != TY_AZOTE_BL_OK && !ignore_invalid)
        {
            fclose(fp);
            return st;
        }
    }
    fclose(fp);
    return TY_AZOTE_BL_OK;
}

static ty_azote_BlStatus apply_array(ty_azote_Constraint *c, const char **entries, size_t len,ty_azote_AddrState state, bool ignore_invalid)
{
    for (size_t i = 0; i < len; ++i)
    {
        ty_azote_BlStatus st = apply_entry(c, entries[i], state);

        if (st != TY_AZOTE_BL_OK && !ignore_invalid)
        {
            return st;
        }
    }
    return TY_AZOTE_BL_OK;
}

ty_azote_BlStatus ty_azote_blocklist_init(ty_azote_Blocklist **out,const ty_azote_BlocklistConfig *cfg)
{
    bool has_allowlist = (cfg->allowlist_file != nullptr) || (cfg->allowlist_entries_len > 0);

    if (cfg->allowlist_file && cfg->allowlist_entries)
    {
        TY_AZOTE_WARN("blocklist: both an allowlist file and inline entries given; using the union of both");
    }
    /* Strategy: if an allowlist is present, deny-by-default; otherwise
     * allow-by-default (mirrors zmap semantics exactly). */
    ty_azote_AddrState default_state = has_allowlist ? TY_AZOTE_ADDR_DISALLOWED : TY_AZOTE_ADDR_ALLOWED;
    ty_azote_Constraint *c = ty_azote_constraint_new(default_state);
    TY_AZOTE_DEBUG("blocklist: default state = %s",default_state == TY_AZOTE_ADDR_ALLOWED ? "allow" : "deny");
    ty_azote_BlStatus st = TY_AZOTE_BL_OK;

    if (has_allowlist)
    {
        if (cfg->allowlist_file)
        {
            st = apply_file(c, cfg->allowlist_file, TY_AZOTE_ADDR_ALLOWED,cfg->ignore_invalid_hosts);

            if (st != TY_AZOTE_BL_OK)
            {
                goto fail;
            }
        }
        if (cfg->allowlist_entries)
        {
            st = apply_array(c, cfg->allowlist_entries, cfg->allowlist_entries_len,TY_AZOTE_ADDR_ALLOWED, cfg->ignore_invalid_hosts);

            if (st != TY_AZOTE_BL_OK)
            {
                goto fail;
            }
        }
    }
    if (cfg->blocklist_file)
    {
        st = apply_file(c, cfg->blocklist_file, TY_AZOTE_ADDR_DISALLOWED,cfg->ignore_invalid_hosts);

        if (st != TY_AZOTE_BL_OK)
        {
            goto fail;
        }
    }
    if (cfg->blocklist_entries)
    {
        st = apply_array(c, cfg->blocklist_entries, cfg->blocklist_entries_len,TY_AZOTE_ADDR_DISALLOWED, cfg->ignore_invalid_hosts);

        if (st != TY_AZOTE_BL_OK)
        {
            goto fail;
        }
    }
    /* 0.0.0.0 is always excluded, matching zmap's hardcoded safety net. */
    ty_azote_constraint_set(c, 0, 32, TY_AZOTE_ADDR_DISALLOWED);
    ty_azote_constraint_compile(c);
    uint64_t allowed = ty_azote_constraint_count(c, TY_AZOTE_ADDR_ALLOWED);
    TY_AZOTE_DEBUG("blocklist: %llu addresses (%.4f%% of IPv4 space) are scannable",(unsigned long long)allowed, allowed * 100.0 / 4294967296.0);

    if (allowed == 0)
    {
        TY_AZOTE_ERROR("blocklist: no addresses are eligible to be scanned with the current configuration");
        ty_azote_constraint_free(c);
        return TY_AZOTE_BL_ERR_NO_ADDRESSES;
    }
    ty_azote_Blocklist *bl = malloc(sizeof *bl);
    bl->constraint = c;
    *out = bl;
    return TY_AZOTE_BL_OK;
    fail:
    ty_azote_constraint_free(c);
    return st;
}

void ty_azote_blocklist_free(ty_azote_Blocklist *bl)
{
    if (!bl)
    {
        return;
    }
    ty_azote_constraint_free(bl->constraint);
    free(bl);
}

bool ty_azote_blocklist_is_allowed(const ty_azote_Blocklist *bl, ty_azote_Ipv4Addr ip)
{
    return ty_azote_constraint_state(bl->constraint, ty_azote_ipv4_to_host(ip)) == TY_AZOTE_ADDR_ALLOWED;
}

uint64_t ty_azote_blocklist_count_allowed(const ty_azote_Blocklist *bl)
{
    return ty_azote_constraint_count(bl->constraint, TY_AZOTE_ADDR_ALLOWED);
}

uint64_t ty_azote_blocklist_count_disallowed(const ty_azote_Blocklist *bl)
{
    return ty_azote_constraint_count(bl->constraint, TY_AZOTE_ADDR_DISALLOWED);
}

ty_azote_Ipv4Addr ty_azote_blocklist_index_to_ip(const ty_azote_Blocklist *bl, uint64_t index)
{
    return ty_azote_ipv4_from_host(ty_azote_constraint_index_to_ip(bl->constraint, index));
}

bool ty_azote_blocklist_ip_to_index(const ty_azote_Blocklist *bl, ty_azote_Ipv4Addr ip,uint64_t *out_index)
{
    return ty_azote_constraint_ip_to_index(bl->constraint, ty_azote_ipv4_to_host(ip), out_index);
}

void ty_azote_blocklist_visit_blocked(const ty_azote_Blocklist *bl,ty_azote_ConstraintVisitFn cb, void *ud)
{
    ty_azote_constraint_visit(bl->constraint, TY_AZOTE_ADDR_DISALLOWED, cb, ud);
}

void ty_azote_blocklist_visit_allowed(const ty_azote_Blocklist *bl,ty_azote_ConstraintVisitFn cb, void *ud)
{
    ty_azote_constraint_visit(bl->constraint, TY_AZOTE_ADDR_ALLOWED, cb, ud);
}








