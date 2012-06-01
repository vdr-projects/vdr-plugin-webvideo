<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template name="type_to_path">
  <xsl:param name="type"/>
  <xsl:choose>
    <xsl:when test="$type = 'audio'">radio</xsl:when>
    <xsl:otherwise>tv</xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
