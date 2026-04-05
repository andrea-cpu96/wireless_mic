
#include "pages.h"

#include <string.h>
#include <stdio.h>

#include "display_drv.h"

/**
 * @brief pages_demo_page
 *
 * @param idx
 */
void pages_demo_page(uint8_t EnDis, uint8_t idx, int v1, int v2, int v3, int v4)
{
        display_pages_t page;

        strcpy(page.title, "DEMO");

        page.EnDis = EnDis;

        strcpy(page.par[0].title, "PAR1");
        strcpy(page.par[1].title, "PAR2");
        strcpy(page.par[2].title, "PAR3");
        strcpy(page.par[3].title, "PAR4");

        snprintf(page.par[0].val, sizeof(page.par[0].val), "%d", v1);
        snprintf(page.par[1].val, sizeof(page.par[1].val), "%d", v2);
        snprintf(page.par[2].val, sizeof(page.par[2].val), "%d", v3);
        snprintf(page.par[3].val, sizeof(page.par[3].val), "%d", v4);

        page.par_select = idx;
        display_drv_pageToShow(page);
        display_drv_event_set(SHOW_PAGE);
}

/**
 * @brief pages_adt_page
 * 
 * @param adt_set 
 * @param idx 
 */
void pages_adt_page(struct adt_settings adt_set, uint8_t idx)
{
        display_pages_t page;

        strcpy(page.title, "ADT");

        page.EnDis = adt_set.EnDis;

        strcpy(page.par[0].title, "DEL");
        strcpy(page.par[1].title, "AMP");
        strcpy(page.par[2].title, "");
        strcpy(page.par[3].title, "");

        snprintf(page.par[0].val, sizeof(page.par[0].val), "%d", adt_set.delay);
        snprintf(page.par[1].val, sizeof(page.par[1].val), "%d", adt_set.fading_lev);
        snprintf(page.par[2].val, sizeof(page.par[2].val), "%c", '\0');
        snprintf(page.par[3].val, sizeof(page.par[3].val), "%c", '\0');

        page.par_select = idx;
        display_drv_pageToShow(page);
        display_drv_event_set(SHOW_PAGE);
}