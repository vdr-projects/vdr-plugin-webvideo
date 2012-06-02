<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
<wvmenu>
  <title><xsl:value-of select="normalize-space(/html/head/meta[@property='og:title']/@content)"/></title>
  <textarea>
    <label><xsl:value-of select="normalize-space(/html/head/meta[@property='og:description']/@content)"/></label>
  </textarea>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
