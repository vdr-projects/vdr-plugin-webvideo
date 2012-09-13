<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
<wvmenu>
  <title>Ruutu.fi</title>

<!--
  <link>
    <label>Haku</label>
    <ref>wvt:///www.ruutu.fi/search.xsl</ref>
  </link>
-->

  <link>
    <label>Listaa sarjat</label>
    <ref>wvt:///www.ruutu.fi/series.xsl?srcurl=http://www.ruutu.fi/ajax/series-navi</ref>
  </link>

  <link>
    <label>Uusimmat</label>
    <ref>wvt:///www.ruutu.fi/recent.xsl?srcurl=http://www.ruutu.fi/etusivu</ref>
  </link>

  <link>
    <label>Katsotuimmat</label>
    <ref>wvt:///www.ruutu.fi/mostviewed.xsl?srcurl=http://www.ruutu.fi/etusivu</ref>
  </link>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
