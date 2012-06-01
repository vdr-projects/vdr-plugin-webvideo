<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="/">
<wvmenu>
  <title>YLE Areena</title>

  <link>
    <label>Haku</label>
    <ref>wvt:///areena.yle.fi/search.xsl</ref>
  </link>

  <!-- TV -->

  <textarea>
    <label>TV</label>
  </textarea>

  <link>
    <label>Kaikki TV-ohjelmat</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/kaikki.json%3Fkpl=100%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,TV-ohjelmat%20A-%C3%96&amp;postprocess=json2xml</ref>
  </link>

  <!-- TV categories -->
  <link>
    <label>Sarjat ja elokuvat</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/sarjat-ja-elokuvat/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Sarjat%20ja%20elokuvat&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Viihde ja kulttuuri</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/viihde-ja-kulttuuri/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Viihde%20ja%20kulttuuri&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Dokumentit ja fakta</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/dokumentit-ja-fakta/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Dokumentit%20ja%20fakta&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Uutiset</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/uutiset/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Uutiset&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Urheilu</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/urheilu/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Urheilu&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Lapset</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/tv/lapset/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Lapset&amp;postprocess=json2xml</ref>
  </link>

  <!-- live -->
  <link>
    <label>Suorat lähetykset</label>
    <ref>wvt:///areena.yle.fi/livebroadcasts.xsl?srcurl=http://areena.yle.fi/tv/suora.json&amp;postprocess=json2xml</ref>
  </link>

  <!-- Radio -->

  <textarea>
    <label>Radio</label>
  </textarea>

  <link>
    <label>Kaikki radio-ohjelmat</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/kaikki.json%3Fkpl=100%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Radio-ohjelmat%20A-%C3%96&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Musiikki ja viihde</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/musiikki-ja-viihde/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Musiikki%20ja%20viihde&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Äänikirjat ja kuunnelmat</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/aanikirjat-ja-kuunnelmat/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,%C3%84%C3%A4nikirjat%20ja%20kuunnelmat&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Fakta ja kulttuuri</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/fakta-ja-kulttuuri/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Kulttuuri&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Uutiset</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/uutiset/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Uutiset&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Urheilu</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/urheilu/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Urheilu&amp;postprocess=json2xml</ref>
  </link>

  <link>
    <label>Lapset</label>
    <ref>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/radio/lapset/kaikki.json%3Fkpl=24%26jarjestys=ao%26sisalto=%26kanava=%26kieli=%26tekstitys=%26sivu=0&amp;param=seriestitle,Lapset&amp;postprocess=json2xml</ref>
  </link>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
