/*-------------------------------------------------------------------------
 * Copyright (C) 1997	National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:		tohdr.c
 * 			Aug  6 1997
 * 			Robb Matzke <robb@maya.nuance.com>
 *
 * Purpose:		
 *
 * Modifications:	
 *
 *-------------------------------------------------------------------------
 */
#include <testhdf5.h>

#include <H5private.h>
#include <H5ACprivate.h>
#include <H5Fprivate.h>
#include <H5Gprivate.h>
#include <H5Oprivate.h>


/*-------------------------------------------------------------------------
 * Function:	test_ohdr
 *
 * Purpose:	Test object headers.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		robb@maya.nuance.com
 *		Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
test_ohdr (void)
{
   hatom_t	fid;
   hdf5_file_t	*f;
   haddr_t	oh;
   H5O_stab_t	stab, ro;
   herr_t	status;
   void		*ptr;
   H5G_entry_t	ent;
   hbool_t	ent_mod;
   int		i;
   
   MESSAGE (5, ("Testing Object Headers\n"));

   /* create the file */
   fid = H5Fcreate ("tohdr.h5", H5ACC_OVERWRITE, 0, 0);
   CHECK (fid, FAIL, "H5Fcreate");
   f = H5Aatom_object (fid);
   CHECK (f, NULL, "H5Aatom_object");

   /* the new object header */
   MESSAGE (8, ("Creating new object header...\n"));
   oh = H5O_new (f, 1, 64);

   /*
    * Test creation of a new message.
    */
   MESSAGE (8, ("Creating new message...\n"));
   stab.btree_addr = 11111111;
   stab.heap_addr  = 22222222;
   status = H5O_modify (f, oh, NULL, NULL, H5O_STAB, H5O_NEW_MESG, &stab);
   VERIFY (status, 0, "H5O_modify");

   H5AC_flush (f, NULL, 0, TRUE);
   ptr = H5O_read (f, oh, NULL, H5O_STAB, 0, &ro);
   CHECK_PTR (ptr, "H5O_read");
   VERIFY (ptr, &ro, "H5O_read");
   VERIFY (ro.btree_addr, stab.btree_addr, "H5O_read");
   VERIFY (ro.heap_addr, stab.heap_addr, "H5O_read");

   /*
    * Test modification of an existing message.
    */
   MESSAGE (8, ("Modifying message...\n"));
   stab.btree_addr = 33333333;
   stab.heap_addr  = 44444444;
   status = H5O_modify (f, oh, NULL, NULL, H5O_STAB, 0, &stab);
   VERIFY (status, 0, "H5O_modify");

   H5AC_flush (f, NULL, 0, TRUE);
   ptr = H5O_read (f, oh, NULL, H5O_STAB, 0, &ro);
   CHECK_PTR (ptr, "H5O_read");
   VERIFY (ptr, &ro, "H5O_read");
   VERIFY (ro.btree_addr, stab.btree_addr, "H5O_read");
   VERIFY (ro.heap_addr, stab.heap_addr, "H5O_read");

   /*
    * Test creation of a second message of the same type with a symbol
    * table.
    */
   MESSAGE (8, ("Creating a duplicate message...\n"));
   ent.header = 0;
   ent.type = H5G_NOTHING_CACHED;
   stab.btree_addr = 55555555;
   stab.heap_addr  = 66666666;
   status = H5O_modify (f, oh, &ent, &ent_mod, H5O_STAB, H5O_NEW_MESG, &stab);
   VERIFY (status, 1, "H5O_modify");
   VERIFY (ent_mod, TRUE, "H5O_modify");
   VERIFY (ent.type, H5G_CACHED_STAB, "H5O_modify");
   VERIFY (ent.cache.stab.heap_addr, stab.heap_addr, "H5O_modify");
   VERIFY (ent.cache.stab.btree_addr, stab.btree_addr, "H5O_modify");

   H5AC_flush (f, NULL, 0, TRUE);
   ptr = H5O_read (f, oh, NULL, H5O_STAB, 1, &ro);
   CHECK_PTR (ptr, "H5O_read");
   VERIFY (ptr, &ro, "H5O_read");
   VERIFY (ro.btree_addr, stab.btree_addr, "H5O_read");
   VERIFY (ro.heap_addr, stab.heap_addr, "H5O_read");

   /*
    * Test modification of the second message with a symbol table.
    */
   MESSAGE (8, ("Modifying the duplicate message...\n"));
   stab.btree_addr = 77777777;
   stab.heap_addr  = 88888888;
   status = H5O_modify (f, oh, &ent, &ent_mod, H5O_STAB, 1, &stab);
   VERIFY (status, 1, "H5O_modify");
   VERIFY (ent_mod, TRUE, "H5O_modify");
   VERIFY (ent.type, H5G_CACHED_STAB, "H5O_modify");
   VERIFY (ent.cache.stab.heap_addr, stab.heap_addr, "H5O_modify");
   VERIFY (ent.cache.stab.btree_addr, stab.btree_addr, "H5O_modify");

   H5AC_flush (f, NULL, 0, TRUE);
   ptr = H5O_read (f, oh, NULL, H5O_STAB, 1, &ro);
   CHECK_PTR (ptr, "H5O_read");
   VERIFY (ptr, &ro, "H5O_read");
   VERIFY (ro.btree_addr, stab.btree_addr, "H5O_read");
   VERIFY (ro.heap_addr, stab.heap_addr, "H5O_read");

   /*
    * Test creation of a bunch of messages one after another to see
    * what happens when the object header overflows in core.
    */
   MESSAGE (8, ("Overflowing header in core...\n"));
   for (i=0; i<40; i++) {
      stab.btree_addr = (i+1)*1000 + 1;
      stab.heap_addr  = (i+1)*1000 + 2;
      status = H5O_modify (f, oh, NULL, NULL, H5O_STAB, H5O_NEW_MESG, &stab);
      VERIFY (status, 2+i, "H5O_modify");
   }
   H5AC_flush (f, NULL, 0, TRUE);

   /*
    * Test creation of a bunch of messages one after another to see
    * what happens when the object header overflows on disk.
    */
   MESSAGE (8, ("Overflowing header on disk...\n"));
   for (i=0; i<10; i++) {
      stab.btree_addr = (i+1)*1000 + 10;
      stab.heap_addr  = (i+1)*1000 + 20;
      status = H5O_modify (f, oh, NULL, NULL, H5O_STAB, H5O_NEW_MESG, &stab);
      VERIFY (status, 42+i, "H5O_modify");
      H5AC_flush (f, NULL, 0, TRUE);
   }

   /*
    * Delete all symbol table messages.
    */
   status = H5O_remove (f, oh, NULL, NULL, H5O_STAB, H5O_ALL);
   CHECK_I (status, "H5O_remove");

   /* close the file */
   H5Fclose (fid);
}
