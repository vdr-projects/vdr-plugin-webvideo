<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:import href="video.xsl"/>

<xsl:param name="docurl"/>

<xsl:template match="/" mode="video_config">
  <xsl:call-template name="mediaurl"/>
</xsl:template>

<xsl:template match="/">
  <xsl:param name="mediatype" select="substring-before(substring-after(id('block-ruutu-commerce-video-area')/script, 'type%3D'), '%26')"/>
  <xsl:param name="videoid" select="substring-before(substring-after(id('block-ruutu-commerce-video-area')/script, 'id%3D'), '%26')"/>
  <xsl:variable name="videourl">http://crossbow.nelonenmedia.fi/mxmlc?type=<xsl:value-of select="$mediatype"/>&amp;id=<xsl:value-of select="$videoid"/>&amp;site=www.ruutu.fi&amp;gRVBR=0</xsl:variable>
  <xsl:apply-templates select="document($videourl)" mode="video_config"/>
</xsl:template>

</xsl:stylesheet>
