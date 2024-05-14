# 
# Convert Brewer data to same RGB format used in color_names.
# See brewer_colors for input format.
#
BEGIN { 
  FS = ",";
}
/^[^#]/{
  if ($1 != "") {
    # Close previous file handle to avoid exhausting file descriptors on macOS
    # https://gitlab.com/graphviz/graphviz/-/issues/1965
    if (name) {
      close(name);
    }
    name = "colortmp/" $1 $2;
    gsub ("\"","",name);
  }
  printf ("%s %s %s %s\n", $5, $7, $8, $9) > name; 
}
