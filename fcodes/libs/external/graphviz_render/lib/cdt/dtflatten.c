#include	<cdt/dthdr.h>
#include	<stddef.h>

/*	Flatten a dictionary into a linked list.
**	This may be used when many traversals are likely.
**
**	Written by Kiem-Phong Vo (5/25/96).
*/

Dtlink_t* dtflatten(Dt_t* dt)
{
	Dtlink_t	*t, *r, *list, *last, **s, **ends;

	/* already flattened */
	if(dt->data->type&DT_FLATTEN )
		return dt->data->here;

	list = last = NULL;
	if(dt->data->type&DT_SET)
	{	for(ends = (s = dt->data->htab) + dt->data->ntab; s < ends; ++s)
		{	if((t = *s) )
			{	if(last)
					last->right = t;
				else	list = last = t;
				while(last->right)
					last = last->right;
				*s = last;
			}
		}
	}
	else if(dt->data->type&DT_QUEUE)
		list = dt->data->head;
	else if((r = dt->data->here) ) /*if(dt->data->type&(DT_OSET|DT_OBAG))*/
	{	while((t = r->left) )
			RROTATE(r,t);
		for(list = last = r, r = r->right; r; last = r, r = r->right)
		{	if((t = r->left) )
			{	do	RROTATE(r,t);
				while((t = r->left) );

				last->right = r;
			}
		}
	}

	dt->data->here = list;
	dt->data->type |= DT_FLATTEN;

	return list;
}
