function max(m,n) {
  return m > n ? m : n;
}
function value (r, g, b) {
  return max(r,max(g,b));
}
function putColor (n, r, g, b, v)
{
  printf ("<td style=\"background-color: #%02x%02x%02x;\" title=\"#%02x%02x%02x\">",r,g,b,r,g,b);
  if (v < 0.51) printf ("<span style=\"color: white;\">%s</span>", n);
  else printf ("%s", n);
  printf ("</td>\n");
}
BEGIN {
  colorsPerRow = 5;
  if (ARGV[1] == "--single-line") {
    ARGV[1] = "";
    name = ARGV[2];
    singleRow = 1;
  }
  else {
    name = "";
    singleRow = 0;
  }
  if (length(name) > 0) {
    sub(".*/","",name);
    printf ("%s color scheme<BR>\n", name);
  }
  printf ("<table class=\"gv-colors\">\n");
}
{
  if (singleRow) idx = NR;
  else idx = NR % colorsPerRow;
  if (idx == 1) printf ("<tr>\n");
  putColor($1,$2,$3,$4,value($2/255.0,$3/255.0,$4/255.0));
  if (idx == 0) printf ("</tr>\n");
}
END {
  if (idx != 0) printf ("</tr>\n");
  printf ("</table>\n");
}
