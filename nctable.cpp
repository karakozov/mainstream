
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <signal.h>
#include <locale.h>
#include <vector>

#ifndef __NCTABLE_H__
#include "nctable.h"
#endif

//-----------------------------------------------------------------------------

table::table(int colomn_number, int cell_width, int cell_height)
{
    init_ncurses();

    m_row = 0;
    m_colomn = colomn_number;
    m_CW = cell_width;
    m_CH = cell_height;
    m_table.clear();

    getmaxyx(stdscr,m_maxH,m_maxW);

    m_W0 = m_maxW/2;
    m_H0 = m_maxH/2;

    m_header.w = NULL;
    m_header.X0 = m_header.Y0 = m_header.W = m_header.H = 0;
    m_status.w = NULL;
    m_status.X0 = m_status.Y0 = m_status.W = m_status.H = 0;
}

//-----------------------------------------------------------------------------

table::table(int row, int col, int cell_width, int cell_height)
{
    init_ncurses();

    m_row = row;
    m_colomn = col;
    m_CW = cell_width;
    m_CH = cell_height;
    m_table.clear();
}

//-----------------------------------------------------------------------------

table::~table()
{
    clear_table();
    clear_header();
    clear_status();
    endwin();
}

//-----------------------------------------------------------------------------

void table::init_ncurses()
{
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush( stdscr, FALSE );
    keypad(stdscr, TRUE);
}

//-----------------------------------------------------------------------------

void table::refresh_table()
{
    for(unsigned i=0; i<m_table.size(); i++) {
        row_t row = m_table.at(i);
        for(unsigned j=0; j<row.w.size(); j++) {
            WINDOW *w = row.w.at(j);
            wrefresh(w);
        }
    }
    if(m_header.w) wrefresh(m_header.w);
    if(m_status.w) wrefresh(m_status.w);
    refresh();
}

//-----------------------------------------------------------------------------

void table::clear_table()
{
    for(unsigned i=0; i<m_table.size(); i++) {
        row_t row = m_table.at(i);
        for(unsigned j=0; j<row.w.size(); j++) {
            WINDOW *w = row.w.at(j);
            delwin(w);
        }
        row.w.clear();
    }
    m_table.clear();
}

//-----------------------------------------------------------------------------

int table::create_table(int nrow, int ncol)
{
    clear_table();

    int dX = m_CW * ncol/2;
    int dY = m_CH * nrow/2;

    for(int row = 0; row < nrow; row++) {

        int x0 = m_W0 - dX;
        int y0 = m_H0 + m_CH * row - dY;

        row_t new_row;

        new_row.X0 = x0;
        new_row.Y0 = y0;

        for(int col = 0; col < ncol; col++) {

            int xn = x0 + m_CW * col;
            int yn = y0;

            WINDOW *w = newwin(m_CH, m_CW, yn, xn);
            if(!w)
                break;
            box(w, 0 , 0);
            wrefresh(w);
            new_row.w.push_back(w);
        }

        m_table.push_back(new_row);
    }

    return m_table.size();
}

//-----------------------------------------------------------------------------

int table::add_row()
{
    int x0 = 0;
    int y0 = 0;

    if(m_table.size()) {

        x0 = m_W0 - m_CW * m_colomn/2;
        y0 = m_H0 - m_H0/2 + m_CH * m_table.size();

    } else {

        x0 = m_W0 - m_CW * m_colomn/2;
        y0 = m_H0 - m_H0/2;
    }

    row_t new_row;

    new_row.X0 = x0;
    new_row.Y0 = y0;

    for(int col = 0; col < m_colomn; col++) {

        int xn = x0 + m_CW * col;
        int yn = y0;

        WINDOW *w = newwin(m_CH, m_CW, yn, xn);
        if(!w)
            break;
            box(w, 0 , 0);
            wrefresh(w);
            new_row.w.push_back(w);
    }

    m_table.push_back(new_row);

    refresh_table();

    return m_table.size() - 1;
}

//-----------------------------------------------------------------------------

int table::set_cell_text(unsigned nrow, unsigned ncol, const char *fmt, ...)
{
    if(nrow >= m_table.size())
        return -1;

    row_t& row = m_table.at(nrow);

    if(ncol >= row.w.size())
        return -2;

    WINDOW *w = row.w.at(ncol);

    if(!w)
        return -3;

    va_list argptr;
    va_start(argptr, fmt);
    char msg[256];
    vsprintf(msg, fmt, argptr);
    wmove(w, m_CH/2, m_CW/2-strlen(msg)/2);
    wprintw(w, "%s", msg);
    wrefresh(w);

    return 0;
}

//-----------------------------------------------------------------------------

bool table::create_header()
{
    if(m_header.w)
        return true;

    if(m_table.empty())
        return false;

    row_t row0 = m_table.at(0);

    m_header.X0 = row0.X0;
    m_header.Y0 = row0.Y0-m_CH;

    m_header.H = m_CH;
    m_header.W = m_CW*row0.w.size();

    WINDOW *w = newwin(m_header.H, m_header.W, m_header.Y0, m_header.X0);
    if(!w) {
        return false;
    }

    m_header.w = w;
    box(w, 0 , 0);

    wrefresh(w);

    return true;
}

//-----------------------------------------------------------------------------

bool table::create_status()
{
    if(m_status.w)
        return true;

    if(m_table.empty())
        return false;

    row_t rowN = m_table.at(m_table.size()-1);

    m_status.X0 = rowN.X0;
    m_status.Y0 = rowN.Y0+m_CH;

    m_status.H = m_CH;
    m_status.W = m_CW*rowN.w.size();

    WINDOW *w = newwin(m_status.H, m_status.W, m_status.Y0, m_status.X0);
    if(!w) {
        return false;
    }

    m_status.w = w;
    box(w, 0 , 0);

    wrefresh(w);

    return true;
}

//-----------------------------------------------------------------------------

int table::set_header_text(const char *fmt, ...)
{
    if(!m_header.w)
        return -1;

    va_list argptr;
    va_start(argptr, fmt);
    char msg[256];
    vsprintf(msg, fmt, argptr);
    wmove(m_header.w, m_header.H/2, m_header.W/2-strlen(msg)/2);
    wprintw(m_header.w, "%s", msg);
    wrefresh(m_header.w);

    return 0;
}

//-----------------------------------------------------------------------------

int table::set_status_text(const char *fmt, ...)
{
    if(!m_status.w)
        return -1;

    va_list argptr;
    va_start(argptr, fmt);
    char msg[256];
    vsprintf(msg, fmt, argptr);
    wmove(m_status.w, m_status.H/2, m_status.W/2-strlen(msg)/2);
    wprintw(m_status.w, "%s", msg);
    wrefresh(m_status.w);

    return 0;
}

//-----------------------------------------------------------------------------

void table::clear_status()
{
    if(m_status.w) {
        delwin(m_status.w);
        m_status.w = NULL;
    }
}

//-----------------------------------------------------------------------------

void table::clear_header()
{
    if(m_header.w) {
        delwin(m_header.w);
        m_header.w = NULL;
    }
}

//-----------------------------------------------------------------------------
