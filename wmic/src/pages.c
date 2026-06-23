
#include "pages.h"

#include <string.h>
#include <stdio.h>

#include "bluetooth_drv.h"
#include "display_drv.h"
#include "signals.h"

static enum pages_e current_page = PEERS_PAGE;

static uint8_t page_changed_flag = 1;

/**
 * @brief switch_page - circular page switch between ADT_PAGE and REC_PAGE
 *
 * @param incDec +1 to go forward, -1 to go backward
 */
static void switch_page(int8_t incDec)
{
    static uint8_t page_num = ADT_PAGE;

    page_num += incDec;

    if (page_num > REC_PAGE)
        page_num = ADT_PAGE;
    else if (page_num < ADT_PAGE)
        page_num = REC_PAGE;

    pages_set_current_page((enum pages_e)page_num);
    page_changed_flag = 1;
}

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
 * @param button_status
 * @param peers_handler
 */
void pages_peers_page(enum buttons_status_e button_status,
                      struct bluetooth_peers_struct *peers_handler)
{
    uint8_t display_flag = 0;

    if (peers_handler->peers_n > 0)
    {
        if ((button_status == BUTTON_RIGHT) || (button_status == BUTTON_UP))
        {
            peers_handler->peer_idex = ((peers_handler->peer_idex + 1) % peers_handler->peers_n);
            display_flag = 1;
        }
        else if ((button_status == BUTTON_LEFT) || (button_status == BUTTON_DOWN))
        {
            peers_handler->peer_idex = (peers_handler->peer_idex == 0) ? (peers_handler->peers_n - 1) : (peers_handler->peer_idex - 1);
            display_flag = 1;
        }
        else if (button_status == BUTTON_SET)
        {
            peers_handler->peer_cb_exit = true; // Exit the peer selection loop in the bluetooth driver
            pages_set_current_page(ADT_PAGE);
            page_changed_flag = 1;
        }
    }

    if (display_flag == 1)
    {
        display_drv_strToShow(peers_handler->peers_p[peers_handler->peer_idex].name);
        display_drv_event_set(SHOW_STRING);
    }
}

/**
 * @brief pages_adt_page
 *
 * @param button_status
 * @param adt_set
 */
void pages_adt_page(enum buttons_status_e button_status,
                    struct adt_settings *adt_set)
{
    static const uint8_t ADT_IDX_NUM = 3;

    uint8_t display_flag = 0;

    display_pages_t page;
    static uint8_t idx = 0;

    display_flag = page_changed_flag;
    page_changed_flag = 0;

    if (button_status == BUTTON_RIGHT)
    {
        if (idx == 0)
            switch_page(1);
        else
            idx = (idx + 1) % ADT_IDX_NUM;
        display_flag = 1;
    }
    else if (button_status == BUTTON_LEFT)
    {
        if (idx == 0)
            switch_page(-1);
        else
            idx = (idx == 0) ? (ADT_IDX_NUM - 1) : (idx - 1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_UP)
    {
        idx = (idx + 1) % ADT_IDX_NUM;
        display_flag = 1;
    }
    else if (button_status == BUTTON_DOWN)
    {
        idx = (idx == 0) ? (ADT_IDX_NUM - 1) : (idx - 1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_SET)
    {
        if (idx == 0)
        {
            adt_set->EnDis = !adt_set->EnDis;
            display_flag = 1;
        }
    }

    if (display_flag)
    {
        strcpy(page.title, "ADT");

        page.EnDis = adt_set->EnDis;

        strcpy(page.par[0].title, "DEL;");
        strcpy(page.par[1].title, "AMP;");
        strcpy(page.par[2].title, "");
        strcpy(page.par[3].title, "");

        snprintf(page.par[0].val, sizeof(page.par[0].val), "%d", adt_set->delay);
        snprintf(page.par[1].val, sizeof(page.par[1].val), "%d", adt_set->fading_lev);
        snprintf(page.par[2].val, sizeof(page.par[2].val), "%c", '\0');
        snprintf(page.par[3].val, sizeof(page.par[3].val), "%c", '\0');

        page.par_select = idx;
        display_drv_pageToShow(page);
        display_drv_event_set(SHOW_PAGE);
    }
}

/**
 * @brief pages_tones_page
 *
 * @param button_status
 * @param tone_set
 */
void pages_tones_page(enum buttons_status_e button_status,
                      struct tone_settings *tone_set)
{
    static enum tone_e tone_previous = TONE_NONE;

    uint8_t display_flag = 0;
    display_pages_t page;

    display_flag = page_changed_flag;
    page_changed_flag = 0;

    if (button_status == BUTTON_RIGHT)
    {
        switch_page(1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_LEFT)
    {
        switch_page(-1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_SET)
    {
        tone_set->EnDis = !tone_set->EnDis;
        display_flag = 1;
    }
    else if (button_status == BUTTON_USER_1)
    {
        tone_set->tone = TONE_500HZ;
        display_flag = 1;
    }
    else if (button_status == BUTTON_USER_2)
    {
        tone_set->tone = TONE_1KHZ;
        display_flag = 1;
    }
    else
    {
        tone_set->tone = TONE_NONE;
    }

    if (tone_previous != tone_set->tone)
    {
        tone_previous = tone_set->tone;
        display_flag = 1;
    }

    if (display_flag)
    {
        strcpy(page.title, "TONES");

        page.EnDis = tone_set->EnDis;

        strcpy(page.par[0].title, "FREQ");
        strcpy(page.par[1].title, "");
        strcpy(page.par[2].title, "");
        strcpy(page.par[3].title, "");

        if (tone_set->tone == TONE_500HZ)
        {
            snprintf(page.par[0].val, sizeof(page.par[0].val), " 500");
        }
        else if (tone_set->tone == TONE_1KHZ)
        {
            snprintf(page.par[0].val, sizeof(page.par[0].val), " 1k");
        }
        else if (tone_set->tone == TONE_3KHZ)
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
}

/**
 * @brief pages_rec_page
 *
 * @param button_status
 * @param rec_set
 */
enum rec_track_id pages_rec_page(enum buttons_status_e button_status, struct rec_settings *rec_set)
{
    static enum buttons_status_e button_previous = BUTTON_NONE;
    static enum rec_track_id track_id_selected = TRACK1;

    uint8_t display_flag = 0;
    display_pages_t page;

    display_flag = page_changed_flag;
    page_changed_flag = 0;

    if (rec_set->EnDis > 0)
    {
        if (button_status != button_previous)
        {
            if (button_status == BUTTON_USER_1)
            {
                if (rec_set->track1 == REC_NONE)
                {
                    rec_set->track1 = REC_START;
                }
                else if (rec_set->track1 == REC_READY)
                {
                    rec_set->track1 = REC_RUN;
                }
                track_id_selected = TRACK1;
                display_flag = 1;
            }

            if (button_status == BUTTON_USER_2)
            {
                if (rec_set->track2 == REC_NONE)
                {
                    rec_set->track2 = REC_START;
                }
                else if (rec_set->track2 == REC_READY)
                {
                    rec_set->track2 = REC_RUN;
                }
                track_id_selected = TRACK2;
                display_flag = 1;
            }

            if (button_status == BUTTON_NONE)
            {
                if (button_previous == BUTTON_USER_1)
                {
                    if ((rec_set->track1 == REC_START) ||
                        (rec_set->track1 == REC_RUN))
                    {
                        rec_set->track1 = REC_READY;
                    }
                    track_id_selected = TRACK1;
                }
                else if (button_previous == BUTTON_USER_2)
                {
                    if ((rec_set->track2 == REC_START) ||
                        (rec_set->track2 == REC_RUN))
                    {
                        rec_set->track2 = REC_READY;
                    }
                    track_id_selected = TRACK2;
                }
                display_flag = 1;
            }
            button_previous = button_status;
        }
    }
    if (button_status == BUTTON_RIGHT)
    {
        switch_page(1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_LEFT)
    {
        switch_page(-1);
        display_flag = 1;
    }
    else if (button_status == BUTTON_SET)
    {
        rec_set->EnDis = !rec_set->EnDis;
        display_flag = 1;
    }

    if (display_flag)
    {
        strcpy(page.title, "REC");

        page.EnDis = rec_set->EnDis;

        strcpy(page.par[0].title, "TRK1;");
        strcpy(page.par[1].title, "TRK2;");
        strcpy(page.par[2].title, "TRK3;");
        strcpy(page.par[3].title, "TRK4;");

        snprintf(page.par[0].val, sizeof(page.par[0].val), "%d", rec_set->track1);
        snprintf(page.par[1].val, sizeof(page.par[1].val), "%d", rec_set->track2);
        snprintf(page.par[2].val, sizeof(page.par[2].val), "%d", rec_set->track3);
        snprintf(page.par[3].val, sizeof(page.par[3].val), "%d", rec_set->track4);

        page.par_select = 0;
        display_drv_pageToShow(page);
        display_drv_event_set(SHOW_PAGE);
    }

    return track_id_selected;
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
