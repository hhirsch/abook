
/*
 * Copyright 2012, Raphaël Droz <raphael.droz+floss@gmail.com>
 *
 * abook's wrapper for libvformat:
 * fits a vcard parsed by libvformat into a usable abook item list
 *
 * see:
 * libvformat's vf_iface.h
 * http://www.imc.org/pdi/vcard-21.txt
 * rfc 2426
 * rfc 2739
 */

#include <stdio.h>
#include <string.h>

#include "database.h"
#include "xmalloc.h"

#include "vcard.h"

int vcard_parse_file_libvformat(char *filename) {
  VF_OBJECT_T*  vfobj;
  if (!vf_read_file(&vfobj, filename)) {
    fprintf(stderr, "Could not read VCF file %s\n", filename);
    return 1;
  }

  // a libvformat property
  VF_PROP_T* prop;
  // property number (used for multivalued properties)
  int props = 0;
  // temporary values
  char *propval;
  char multival[MAX_FIELD_LEN] = { 0 };
  size_t available = MAX_FIELD_LEN;

  do {
    list_item item = item_create();

    // fullname [ or struct-name [ or name ] ]
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "FN", NULL))
      if ((propval = vf_get_prop_value_string(prop, 0)))
	item_fput(item, NAME, xstrdup(propval));

    if (!propval && vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "N", "*", NULL)) {
      // TODO: GIVENNAME , FAMILYNAME
      propval = vf_get_prop_value_string(prop, 0);
      if(propval)
	item_fput(item, NAME, xstrdup(propval));
    }

    if (!propval && vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "NAME", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      if(propval)
	item_fput(item, NAME, xstrdup(propval));
    }

    // email(s)
    // TODO: use our strconcat() ?
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "EMAIL", NULL)) {
      props = 0;
      available = MAX_FIELD_LEN;
      while (available > 0 && props < 5) {
	propval = vf_get_prop_value_string(prop, props++);
	if(!propval) continue;
	if (available > 0 && *multival != 0)
	  strncat(multival, ",", available--);
	strncat(multival, propval, available);
	available -= strlen(propval);
      }
      if (available < MAX_FIELD_LEN)
	item_fput(item, EMAIL, xstrdup(multival));
    }

    // format for ADR:
    // PO Box, Extended Addr, Street, Locality, Region, Postal Code, Country
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "ADR", NULL)) {
      props = 0;
      // US address ?
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, ADDRESS, xstrdup(propval));
      // address
      propval = vf_get_prop_value_string(prop, props++);
      // TODO: concat ?

      // street: TODO: address1 instead ?
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, ADDRESS2, xstrdup(propval));
      // city
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, CITY, xstrdup(propval));
      // state
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, STATE, xstrdup(propval));
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, ZIP, xstrdup(propval));
      propval = vf_get_prop_value_string(prop, props++);
      if(propval)
	item_fput(item, COUNTRY, xstrdup(propval));
    }


    /*
    // city: not in libvformat
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "ADR", "CITY", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, CITY, xstrdup(propval));
    }
    // state
    // zip
    // country
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "C", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, COUNTRY, xstrdup(propval));
    }
    */

    // phone
    // check for HOME
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "TEL", "HOME")) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, PHONE, xstrdup(propval));
    }
    // or grab a more generic one otherwise
    else if(vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "TEL", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, PHONE, xstrdup(propval));
    }

    // workphone
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "TEL", "WORK", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, WORKPHONE, xstrdup(propval));
    }

    // fax
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "TEL", "FAX", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, FAX, xstrdup(propval));
    }

    // cellphone
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "TEL", "CELL", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, MOBILEPHONE, xstrdup(propval));
    }

    // nick
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "NICKNAME", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, NICK, xstrdup(propval));
    }

    // url
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "URL", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, URL, xstrdup(propval));
    }

    // notes
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "NOTE", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, NOTES, xstrdup(propval));
    }

    // anniversary
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "BDAY", NULL)) {
      propval = vf_get_prop_value_string(prop, 0);
      item_fput(item, ANNIVERSARY, xstrdup(propval));
    }

    // (mutt) groups
    if (vf_get_property(&prop, vfobj, VFGP_FIND, NULL, "CATEGORIES", NULL)) {
	props = 0;
	available = MAX_FIELD_LEN;
	*multival = 0;
	while (available > 0 && props < 5) {
	  propval = vf_get_prop_value_string(prop, props++);
	  if(!propval) continue;
	  if (available > 0 && *multival != 0)
	    strncat(multival, ",", available--);
	  strncat(multival, propval, available);
	  available -= strlen(propval);
	}
	if (available < MAX_FIELD_LEN)
	  item_fput(item, GROUPS, xstrdup(multival));
    }

    add_item2database(item);
    item_free(&item);
  } while (vf_get_next_object(&vfobj));

  return 0;
}
