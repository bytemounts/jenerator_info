using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace generator_web.Migrations
{
    /// <inheritdoc />
    public partial class deneme : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "generator_datas",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    CalismaDurumu = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    OperationMode = table.Column<int>(type: "int", nullable: false),
                    SistemCalismaSuresi = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l1 = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l2 = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l3 = table.Column<int>(type: "int", nullable: false),
                    SebekeHz = table.Column<int>(type: "int", nullable: false),
                    ToplamGuc = table.Column<int>(type: "int", nullable: false),
                    SebekeDurumu = table.Column<bool>(type: "bit", nullable: false),
                    GenVoltaj_l1 = table.Column<int>(type: "int", nullable: false),
                    GenVoltaj_l2 = table.Column<int>(type: "int", nullable: false),
                    GenVoltaj_l3 = table.Column<int>(type: "int", nullable: false),
                    GenHz = table.Column<int>(type: "int", nullable: false),
                    GenUretilenGuc = table.Column<int>(type: "int", nullable: false),
                    GenGucFaktoru = table.Column<int>(type: "int", nullable: false),
                    MotorRpm = table.Column<int>(type: "int", nullable: false),
                    MotorSicaklik = table.Column<int>(type: "int", nullable: false),
                    YagBasinci = table.Column<int>(type: "int", nullable: false),
                    YakitSeviyesi = table.Column<int>(type: "int", nullable: false),
                    BataryaVoltaji = table.Column<int>(type: "int", nullable: false),
                    timestamp = table.Column<int>(type: "int", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_generator_datas", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "user_datas",
                columns: table => new
                {
                    userId = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    command = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    DateTime = table.Column<DateTime>(type: "datetime2", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_user_datas", x => x.userId);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "generator_datas");

            migrationBuilder.DropTable(
                name: "user_datas");
        }
    }
}
