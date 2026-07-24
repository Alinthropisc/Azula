#include "rawsock.h"
#include "../../masscan-master/src/rawsock-adapter.h"

/***************************************************************************
 ***************************************************************************/
int
stack_if_datalink(struct Adapter *adapter)
{
    if (adapter->ring)
        return 1; /* ethernet */
    else {
        return adapter->link_type;
    }
}
