<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="/">
  <mediaurl>
    <!-- Jakson/ohjelman nimi -->
    <title><xsl:value-of select="normalize-space(/html/head/meta[@property='og:title']/@content)"/></title>
    <url priority="50"><xsl:value-of select="substring-before(substring-after(/html/head/meta[@property='og:video']/@content, 'file='), '&amp;')"/></url>
  </mediaurl>
</xsl:template>

</xsl:stylesheet>
