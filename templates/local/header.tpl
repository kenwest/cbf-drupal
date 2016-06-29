{if $host_name == "smithlecture.org"}
  {include file="local/smith_header.tpl" }
{elseif $host_name == "citywoman.org.au"}
  {include file="local/citywoman_header.tpl" }
{elseif $host_name == "gpb.citybibleforum.org"}
  {include file="local/gpb_header.tpl" }
{else}
  {include file="local/citybibleforum_header.tpl" }
{/if}
