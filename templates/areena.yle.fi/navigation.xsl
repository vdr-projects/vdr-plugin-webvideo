<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:include href="common.xsl"/>

<xsl:param name="docurl"/>
<xsl:param name="seriestitle"/>
<xsl:param name="preferseries" select="'true'"/> <!-- Shows series when available instead of program -->

<!-- Add previous and next links for a navigation page.
     
     BUG: If the last page has exactly $numitems, an extra "next" link
     is inserted.
-->
<xsl:template name="prevnextlinks">
  <xsl:param name="page" select="0"/>     <!-- current page number -->
  <xsl:param name="numitems" select="0"/> <!-- number of items on this page -->
  <xsl:param name="maxitems" select="0"/> <!-- expected number of items per page -->

  <xsl:if test="string(number($page)) != 'NaN'">
    <xsl:variable name="urlprefix" select="concat(substring-before($docurl, '&amp;sivu='), '&amp;sivu=')"/>
    <xsl:variable name="urlpostfix0" select="substring-after(substring-after($docurl, '&amp;sivu='), '&amp;')"/>
    <xsl:variable name="urlpostfix">
      <xsl:if test="urlpostfix0 != ''">
        <xsl:value-of select="concat('&amp;', urlpostfix0)"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="prevurl" select="concat($urlprefix, $page - 1, $urlpostfix)"/>
    <xsl:variable name="nexturl" select="concat($urlprefix, $page + 1, $urlpostfix)"/>

    <!-- prev link -->
    <xsl:if test="$page &gt; 0">
      <link>
        <label>Edellinen</label>
        <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=<xsl:value-of select="str:encode-uri($prevurl, true())"/>&amp;param=seriestitle,<xsl:value-of select="$seriestitle"/>&amp;param=preferseries,<xsl:value-of select="$preferseries"/>&amp;postprocess=json2xml</ref>
      </link>
    </xsl:if>

    <!-- next link -->
    <xsl:if test="$numitems &gt;= $maxitems">
      <link>
        <label>Seuraava</label>
        <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=<xsl:value-of select="str:encode-uri($nexturl, true())"/>&amp;param=seriestitle,<xsl:value-of select="$seriestitle"/>&amp;param=preferseries,<xsl:value-of select="$preferseries"/>&amp;postprocess=json2xml</ref>
      </link>
    </xsl:if>

  </xsl:if>
</xsl:template>


<xsl:template match="results/list/li">
  <xsl:variable name="typeurl">
    <xsl:call-template name="type_to_path">
      <xsl:with-param name="type" select="dict/type"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="seriestitle2">
    <xsl:choose>
      <xsl:when test="dict/series/dict/name != ''">
        <xsl:value-of select="dict/series/dict/name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="dict/title"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="dict/series and $preferseries = 'true'">
      <!-- series link -->
      <link>
        <label><xsl:value-of select="$seriestitle2"/></label>
        <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=<xsl:value-of select="str:encode-uri(concat('http://areena.yle.fi/', $typeurl, '/', dict/series/dict/id, '.json?kpl=16&amp;sisalto=ohjelmat&amp;sivu=0'), true())"/>&amp;param=seriestitle,<xsl:value-of select="str:encode-uri($seriestitle2, true())"/>&amp;param=preferseries,false&amp;postprocess=json2xml</ref>
      </link>
    </xsl:when>

    <xsl:otherwise>
      <xsl:variable name="streamtitle">
        <xsl:choose>
	  <xsl:when test="dict/series/dict/name = dict/title">
	    <xsl:value-of select="concat(dict/title, ' osa ', dict/episodeNumber)"/>
	  </xsl:when>
          <xsl:when test="dict/series/dict/name != ''">
            <xsl:value-of select="concat(dict/series/dict/name, ': ', dict/title)"/>
          </xsl:when>
          <xsl:when test="starts-with(dict/title, $seriestitle)">
            <xsl:value-of select="concat(dict/title, '-', dict/published)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="dict/title"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <link>
        <label><xsl:value-of select="$streamtitle"/></label>
        <ref>wvt:///areena.yle.fi/description.xsl?srcurl=<xsl:value-of select="str:encode-uri(concat('http://areena.yle.fi/', $typeurl, '/', dict/id, '.json'), true())"/>&amp;param=title,<xsl:value-of select="str:encode-uri($streamtitle, true())"/>&amp;postprocess=json2xml</ref>
        <stream>wvt:///areena.yle.fi/video.xsl?srcurl=&amp;param=title,<xsl:value-of select="str:encode-uri($streamtitle, true())"/>&amp;param=type,<xsl:value-of select="dict/type"/>&amp;param=id,<xsl:value-of select="dict/id"/></stream>
      </link>
      </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title><xsl:value-of select="$seriestitle"/></title>

  <xsl:apply-templates select="/jsondocument/dict/search/dict/results/list/li"/>

  <xsl:call-template name="prevnextlinks">
    <xsl:with-param name="page" select="number(jsondocument/dict/search/dict/params/dict/sivu)"/>
    <xsl:with-param name="numitems" select="count(jsondocument/dict/search/dict/results/list/li)"/>
    <xsl:with-param name="maxitems" select="number(jsondocument/dict/search/dict/params/dict/kpl)"/>
  </xsl:call-template>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
