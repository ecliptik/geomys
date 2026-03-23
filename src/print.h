/*
 * print.h - Printing support for Geomys
 */

#ifndef PRINT_H
#define PRINT_H

#ifdef GEOMYS_PRINT

/* Page Setup dialog — configure paper/printer */
void do_page_setup(void);

/* Print current page */
void do_print(void);

#endif /* GEOMYS_PRINT */
#endif /* PRINT_H */
