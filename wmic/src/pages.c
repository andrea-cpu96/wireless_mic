
#include "pages.h"

#include <string.h>
#include <stdio.h>

#include "display_drv.h"

static enum pages_e current_page = PEERS_PAGE;

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
 * @brief pages_peers_page
 *
 * @param peer_name
 */
void pages_peers_page(const char *peer_name)
{
	display_drv_strToShow(peer_name);
	display_drv_event_set(SHOW_STRING);
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

	strcpy(page.par[0].title, "DEL;");
	strcpy(page.par[1].title, "AMP;");
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

/**
 * @brief pages_tones_page
 *
 * @param tone_set
 */
void pages_tones_page(struct tone_settings tone_set)
{
	display_pages_t page;

	strcpy(page.title, "TONES");

	page.EnDis = tone_set.EnDis;

	strcpy(page.par[0].title, "FREQ");
	strcpy(page.par[1].title, "");
	strcpy(page.par[2].title, "");
	strcpy(page.par[3].title, "");

	if (tone_set.tone == TONE_500HZ)
	{
		snprintf(page.par[0].val, sizeof(page.par[0].val), " 500");
	}
	else if (tone_set.tone == TONE_1KHZ)
	{
		snprintf(page.par[0].val, sizeof(page.par[0].val), " 1k");
	}
	else if (tone_set.tone == TONE_3KHZ)
	{
		snprintf(page.par[0].val, sizeof(page.par[0].val), " 3k");
	}
	else
	{
		snprintf(page.par[0].val, sizeof(page.par[0].val), " NO");
	}

	snprintf(page.par[1].val, sizeof(page.par[1].val), "%c", '\0');
	snprintf(page.par[2].val, sizeof(page.par[2].val), "%c", '\0');
	snprintf(page.par[3].val, sizeof(page.par[3].val), "%c", '\0');

	page.par_select = 0;
	display_drv_pageToShow(page);
	display_drv_event_set(SHOW_PAGE);
}

/**
 * @brief pages_rec_page
 *
 * @param rec_set
 */
void pages_rec_page(struct rec_settings rec_set)
{
	display_pages_t page;

	strcpy(page.title, "REC");

	page.EnDis = rec_set.EnDis;

	strcpy(page.par[0].title, "TRK1;");
	strcpy(page.par[1].title, "TRK2;");
	strcpy(page.par[2].title, "TRK3;");
	strcpy(page.par[3].title, "TRK4;");

	snprintf(page.par[0].val, sizeof(page.par[0].val), "%d", rec_set.track1);
	snprintf(page.par[1].val, sizeof(page.par[1].val), "%d", rec_set.track2);
	snprintf(page.par[2].val, sizeof(page.par[2].val), "%d", rec_set.track3);
	snprintf(page.par[3].val, sizeof(page.par[3].val), "%d", rec_set.track4);

	page.par_select = 0;
	display_drv_pageToShow(page);
	display_drv_event_set(SHOW_PAGE);
}

/**
 * @brief set_current_page
 *
 * @param page
 */
void pages_set_current_page(enum pages_e page)
{
	current_page = page;
}

/**
 * @brief get_current_page
 *
 * @return enum pages_e
 */
enum pages_e pages_get_current_page(void)
{
	return current_page;
}
