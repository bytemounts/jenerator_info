using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace generator_web.Migrations
{
    /// <inheritdoc />
    public partial class generator_Data_update : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<bool>(
                name: "KapatmaAlarmi",
                table: "generator_datas",
                type: "bit",
                nullable: false,
                defaultValue: false);

            migrationBuilder.AddColumn<bool>(
                name: "SistemSaglikli",
                table: "generator_datas",
                type: "bit",
                nullable: false,
                defaultValue: false);

            migrationBuilder.AddColumn<bool>(
                name: "UyariAlarmi",
                table: "generator_datas",
                type: "bit",
                nullable: false,
                defaultValue: false);

            migrationBuilder.AddColumn<bool>(
                name: "YukAtmaAlarmi",
                table: "generator_datas",
                type: "bit",
                nullable: false,
                defaultValue: false);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "KapatmaAlarmi",
                table: "generator_datas");

            migrationBuilder.DropColumn(
                name: "SistemSaglikli",
                table: "generator_datas");

            migrationBuilder.DropColumn(
                name: "UyariAlarmi",
                table: "generator_datas");

            migrationBuilder.DropColumn(
                name: "YukAtmaAlarmi",
                table: "generator_datas");
        }
    }
}
