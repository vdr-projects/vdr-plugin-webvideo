<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
<wvmenu>
  <title>Vimeo</title>

  <link>
    <label>Search</label>
    <ref>wvt:///vimeo/search.xsl?srcurl=http://www.vimeo.com/</ref>
  </link>

  <link>
    <label>Channels</label>
    <ref>wvt:///vimeo/channels.xsl?srcurl=http://www.vimeo.com/channels/all</ref>
  </link>

  <link>
    <label>Groups</label>
    <ref>wvt:///vimeo/groups.xsl?srcurl=http://www.vimeo.com/groups/all</ref>
  </link>

</wvmenu>
</xsl:template>

</xsl:stylesheet>
