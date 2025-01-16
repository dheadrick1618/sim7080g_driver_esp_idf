local current_dir = vim.fn.expand("%:p:h")
local project_root = vim.fn.fnamemodify(current_dir, ":h:h")

-- Debug outputs
print("Current directory:", current_dir)
print("Project root:", project_root)
print("Compilation database path:", project_root .. "/build/compile_commands.json")

require("lspconfig").clangd.setup({
	cmd = {
		"clangd",
		"--background-index",
		"--compile-commands-dir=" .. project_root .. "/build",
		"--header-insertion=iwyu",
		"--completion-style=detailed",
		"--enable-config",
	},
	root_dir = function()
		-- Set the root directory to the main project directory
		return project_root
	end,
	on_attach = function(client, bufnr)
		-- Print debugging info when LSP attaches
		print("Clangd attached with root_dir:", client.config.root_dir)
		print("Buffer path:", vim.api.nvim_buf_get_name(bufnr))
	end,
})

-- Configure include paths for ESP-IDF
local esp_idf_path = os.getenv("IDF_PATH")
if esp_idf_path then
	vim.opt.path:append(esp_idf_path .. "/components/**")
end
