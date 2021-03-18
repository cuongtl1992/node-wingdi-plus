{
  "targets": [
    {
      "target_name": "drawer",
      "sources": [ "drawer.cc" ],
	    "include_dirs": ["<!(node -e \"require('nan')\")"],
      "libraries": [ "Gdiplus.lib", "Shlwapi.lib"]
    }
  ]
}
