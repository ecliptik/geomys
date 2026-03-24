/*
 * Balloons.r - Minimal Balloon Help type definitions for Retro68
 *
 * Retro68 does not ship Apple's Balloons.r, so we define the
 * subset needed for hmnu (Help Menu) resources. Based on the
 * hmnu resource format from Inside Macintosh: More Macintosh
 * Toolbox, Chapter 3 (Help Manager).
 */

#ifndef _BALLOONS_R_
#define _BALLOONS_R_

#define HelpMgrVersion      2
#define hmDefaultOptions     0

type 'hmnu'
{
	integer;                          /* version (HelpMgrVersion = 2) */
	longint;                          /* options (hmDefaultOptions = 0) */
	integer;                          /* procID */
	integer;                          /* variant */

	/* Missing items specification — shown when no hmnu entry
	 * exists for dynamically appended items (DAs, etc.) */
	switch {
		case HMSkipItem:
			key integer = 256;

		case HMStringItem:
			key integer = 1;
			pstring;              /* enabled message */
			pstring;              /* dimmed message */
			pstring;              /* checked message */
			pstring;              /* other message */
	};

	/* Menu title + item balloons */
	integer = $$CountOf(MenuArray);
	array MenuArray {
		switch {
			case HMSkipItem:
				key integer = 256;

			case HMStringItem:
				key integer = 1;
				pstring;      /* enabled message */
				pstring;      /* dimmed message */
				pstring;      /* checked message */
				pstring;      /* other message */
		};
		align word;
	};
};

#endif /* _BALLOONS_R_ */
